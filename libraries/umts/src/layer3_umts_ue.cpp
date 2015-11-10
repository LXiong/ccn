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
#include <list>
#include <iostream>
#include <queue>

#include "api.h"
#include "partition.h"
#include "network.h"
#include "network_ip.h"
#include "stats_net.h"

#include "cellular.h"
#include "cellular_layer3.h"
#include "cellular_umts.h"
#include "layer3_umts.h"
#include "layer3_umts_ue.h"
#include "app_umtscall.h"
#include "layer2_umts.h"
#include "layer2_umts_rlc.h"
#include "layer2_umts_mac.h"
#include "layer2_umts_mac_phy.h"

#define DEBUG_SPECIFIED_NODE_ID 0 // DEFAULT sh oud be 0
#define DEBUG_SINGLE_NODE   (node->nodeId == DEBUG_SPECIFIED_NODE_ID)
#define DEBUG_PARAMETER     (0 || (0 &&  DEBUG_SINGLE_NODE))
#define DEBUG_RRC_RB        (0 || (0 &&  DEBUG_SINGLE_NODE))
#define DEBUG_RRC_TRCH      (0 || (0 &&  DEBUG_SINGLE_NODE))
#define DEBUG_RRC_RL        (0 || (0 &&  DEBUG_SINGLE_NODE))
#define DEBUG_MEASUREMENT   (0 || (0 &&  DEBUG_SINGLE_NODE))
#define DEBUG_RR            (0 || (0 &&  DEBUG_SINGLE_NODE))
#define DEBUG_MM            (0 || (0 &&  DEBUG_SINGLE_NODE))
#define DEBUG_GMM           (0 || (0 &&  DEBUG_SINGLE_NODE))
#define DEBUG_CC            (0 || (0 &&  DEBUG_SINGLE_NODE))
#define DEBUG_SM            (0 || (0 &&  DEBUG_SINGLE_NODE))
#define DEBUG_CELL_SELECT   (0 || (0 &&  DEBUG_SINGLE_NODE))
#define DEBUG_HO            (0 || (0 &&  DEBUG_SINGLE_NODE))
#define DEBUG_RR_RELEASE    (0 || (0 &&  DEBUG_SINGLE_NODE))
#define DEBUG_QOS           (0 || (0 &&  DEBUG_SINGLE_NODE))
#define DEBUG_ASSERT         0  // some ERROR_Assert will be triggered
#define DEBUG_RB_HSDPA       0

// /**
// FUNCTION   :: UmtsMmUeUpdateMmState
// LAYER      :: Layer3 MM
// PURPOSE    :: Handle MM state update
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data*   : Pointer to L3 data
// + ueL3             : UmtsLayer3Ue *    : Pointer to l3 Ue data
// + eventType        : UmtsMmStateUpdateEventType : MM event type
// RETURN     :: void : NULL
// **/
static
void UmtsMmUeUpdateMmState(Node* node,
                           UmtsLayer3Data* umtsL3,
                           UmtsLayer3Ue* ueL3,
                           UmtsMmStateUpdateEventType eventType);

//
// RRC notification function interface for NAS stratum
//
// /**
// FUNCTION   :: UmtsLayer3UeInitRrcConnReq
// LAYER      :: Layer3 RRC
// PURPOSE    :: Initiate RRC connection request by MM/GMM
// PARAMETERS ::
// +node      : Node*               : Pointer to node.
// +umtsL3    : UmtsLayer3Data *    : Pointer to UMTS Layer3 data
// +cause     : UmtsRrcEstCause     : Establishment Cause
// +domainIndi: UmtsLayer3CnDomainId: Domain indicator
// +connected : BOOL    :  For returning if there is RRC conn already
// RETURN     :: BOOL               : TRUE if RRC conn req has been sent
// **/
static
BOOL UmtsLayer3UeInitRrcConnReq(Node* node,
                                UmtsLayer3Data *umtsL3,
                                UmtsRrcEstCause cause,
                                UmtsLayer3CnDomainId domainIndi,
                                BOOL* connected);

// /**
// FUNCTION   :: UmtsLayer3UeRrcConnEstInd
// LAYER      :: Layer3 RRC
// PURPOSE    :: Notify MM/GMM whether the RRC conn. has been established
// PARAMETERS ::
// +node      : Node*               : Pointer to node.
// +umtsL3    : UmtsLayer3Data *    : Pointer to UMTS Layer3 data
// +BOOL      : success             : Whether RRC connection has
//                                    been established or not
// RETURN     :: NULL
// **/
static
void UmtsLayer3UeRrcConnEstInd(Node* node,
                               UmtsLayer3Data *umtsL3,
                               BOOL  success);

// FUNCTION   :: UmtsLayer3UeRrcConnRelInd
// LAYER      :: Layer3 RRC
// PURPOSE    :: Notify MM/GMM the RRC connection has been released
// PARAMETERS ::
// + node      : Node*               : Pointer to node.
// + umtsL3    : UmtsLayer3Data *    : Pointer to UMTS Layer3 data
// RETURN     :: NULL
// **/
static
void UmtsLayer3UeRrcConnRelInd(Node* node,
                               UmtsLayer3Data *umtsL3);

// /**
// FUNCTION   :: UmtsLayer3UeMmConnEstInd
// LAYER      :: Layer3 RRC
// PURPOSE    :: Notify CM whether the MM connection has been established
// PARAMETERS ::
// +node      : Node*               : Pointer to node.
// +umtsL3    : UmtsLayer3Data *    : Pointer to UMTS Layer3 data
// +pd        : UmtsLayer3PD        : Protocol discriminator
// +BOOL      : success             : Whether RRC connection has
//                                    been established or not
// RETURN     :: NULL
// **/
static
void UmtsLayer3UeMmConnEstInd(Node* node,
                              UmtsLayer3Data *umtsL3,
                              UmtsLayer3PD pd,
                              unsigned char ti,
                              BOOL  success);

// /**
// FUNCTION   :: UmtsLayer3UeMmConnRelInd
// LAYER      :: Layer3 RRC
// PURPOSE    :: Notify CM an active MM connection is released
// PARAMETERS ::
// + node      : Node*               : Pointer to node.
// + umtsL3    : UmtsLayer3Data *    : Pointer to UMTS Layer3 data
// + pd        : UmtsLayer3PD        : Protocol discriminator
// + BOOL      : success             : Whether RRC connection has
//                                    been established or not
// RETURN     :: NULL
// **/
static
void UmtsLayer3UeMmConnRelInd(Node* node,
                              UmtsLayer3Data *umtsL3,
                              UmtsLayer3PD pd,
                              unsigned char ti);

// FUNCTION   :: UmtsLayer3UeHandleNasMsg
// LAYER      :: Layer3 RRC
// PURPOSE    :: Notify MM/GMM the arrival of NAS message
//               Note: user do not try to release the buffer containing
//               the NAS message. If need to store the message, keep a
//               copy, since the message will be released after the function
//               returns
// PARAMETERS ::
// + node      : Node*               : Pointer to node.
// + umtsL3    : UmtsLayer3Data *    : Pointer to UMTS Layer3 data
// + nasMsg    : char*               : Pointer to the NAS message content
// + nasMsgSize: int                 : NAS message size
// + domain    : UmtsLayer3CnDomainId: Domain indicator
// RETURN     :: NULL
// **/
static
void UmtsLayer3UeHandleNasMsg(Node* node,
                              UmtsLayer3Data *umtsL3,
                              char* nasMsg,
                              int   nasMsgSize,
                              UmtsLayer3CnDomainId domain);

#if 0
static
void UmtsLayer3UeIndPagingToNas(Node* node,
                                UmtsLayer3Data *umtsL3,
                                UmtsLayer3CnDomainId domain,
                                UmtsPagingCause cause);
#endif

// FUNCTION   :: UmtsLayer3UeSendNasMsg
// LAYER      :: Layer3 RRC
// PURPOSE    :: Requested by upper layer to send NAS message.
//               Note: RRC connection must be established before this
//               function is called
// PARAMETERS ::
// + node      : Node*               : Pointer to node.
// + umtsL3    : UmtsLayer3Data *    : Pointer to UMTS Layer3 data
// + msg       : char*               : Pointer to the NAS message content
// + nasmsgSize   : int              : NAS message size
// + pd        : char                : Protocol discriminator
// + msgType   : char                : NAS message type
// + transctId : unsigned char                : Transaction ID
// + domain    : UmtsLayer3CnDomainId: Domain indicator
// RETURN     :: BOOL               : TRUE if the message has been sent
// **/
static
BOOL UmtsLayer3UeSendNasMsg(Node* node,
                            UmtsLayer3Data *umtsL3,
                            char* nasMsg,
                            int   nasMsgSize,
                            char  pd,
                            char  msgType,
                            unsigned char  transactId,
                            UmtsLayer3CnDomainId domain);

// /**
// FUNCTION   :: UmtsLayer3UeSmSendActivatePDPContextRequest
// LAYER      :: Layer 3
// PURPOSE    :: Send PDP activation request
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + nodeInput : const NodeInput* : Pointer to node input.
// + umtsL3    : UmtsLayer3Data*  : Pint to the umts L3 data
// + ueL3      : UmtsLayer3Ue *   : Pointer to UMTS UE Layer3 data
// + srcDestType : UmtsFlowSrcDestType : PDP context source/destination
// + transId   : unsigned char    : transaction Id
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeSmSendActivatePDPContextRequest(
                             Node* node,
                             UmtsLayer3Data *umtsL3,
                             UmtsLayer3Ue* ueL3,
                             UmtsFlowSrcDestType srcDestType,
                             unsigned char transId);

// /**
// FUNCTION   :: UmtsUeInitLocationUpdate
// LAYER      :: Layer3 MM
// PURPOSE    :: Init Loction Update
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data*   : Pointer to L3 data
// + ueL3             : UmtsLayer3Ue *    : Pointer to l3 Ue data
// + locUpdateType    : UmtsLocUpdateType : Location update type
// RETURN     :: void : NULL
// **/
static
void UmtsMmUeInitLocationUpdate(
                         Node* node,
                         UmtsLayer3Data* umtsL3,
                         UmtsLayer3Ue* ueL3,
                         UmtsLocUpdateType locUpdate);

// /**
// FUNCTION   :: UmtsLayer3UeGetCellStatusSetSize
// LAYER      :: Layer 3
// PURPOSE    :: Get the size of Status Set Size of the UE
// + node      : Node*             : Pointer to node.
// + umtsL3    : UmtsLayer3Data *  : Pointer to the umts layer3 data
// + ueL3      : UmtsLayer3Ue*     : Pointer to the UE umts layer3 data
// + cellStatus : UmtsUeCellStatus : cell status
// RETURN     :: int : the size of the status set
// **/
static
int UmtsLayer3UeGetCellStatusSetSize(
        Node* node,
        UmtsLayer3Data *umtsL3,
        UmtsLayer3Ue* ueL3,
        UmtsUeCellStatus cellStatus)
{
    std::list <UmtsLayer3UeNodebInfo*>::iterator it;
    int i = 0;

    for (it = ueL3->ueNodeInfo->begin();
            it != ueL3->ueNodeInfo->end(); ++it)
    {
        if ((*it)->cellStatus == cellStatus)
        {
            i ++;
        }
    }

    return i;
}

// /**
// FUNCTION   :: UmtsLayer3UeGetData
// LAYER      :: Layer 3
// PURPOSE    :: Get the pointer which points to the UMTS layer 3 data.
// PARAMETERS ::
// + node      : Node *           : Pointer to node.
// RETURN     :: UmtsLayer3Data * : Pointer to UMTS layer3 data or NULL
// **/
static
UmtsLayer3Data* UmtsLayer3UeGetData(Node *node)
{
    CellularLayer3Data *cellularData;

    cellularData = (CellularLayer3Data *)
                   node->networkData.cellularLayer3Var;

    ERROR_Assert(cellularData != NULL, "UMTS: Memory error!");
    ERROR_Assert(cellularData->umtsL3 != NULL, "UMTS: Memory error!");

    return cellularData->umtsL3;
}

// /**
// FUNCTION   :: UmtsLayer3UeGetNodeBData
// LAYER      :: Layer 3
// PURPOSE    :: Get the pointer which points to the NodeB layer 3 data.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data*   : Pointer to L3 data
// + ueL3             : UmtsLayer3Ue *    : Pointer to l3 Ue data
// + cellId           : UInt32            : cellId
// RETURN     :: UmtsLayer3Data * : Pointer to UMTS layer3 data or NULL
// **/
static
UmtsLayer3UeNodebInfo* UmtsLayer3UeGetNodeBData(
                           Node* node,
                           UmtsLayer3Data* umtsL3,
                           UmtsLayer3Ue* ueL3,
                           UInt32 cellId)
{
    UmtsLayer3UeNodebInfo* nodeBInfo = NULL;

    std::list <UmtsLayer3UeNodebInfo*>::iterator it;
    for (it = ueL3->ueNodeInfo->begin();
            it != ueL3->ueNodeInfo->end();
            it ++)
    {
        if ((*it)->cellId == cellId)
        {
            nodeBInfo = *it;
            break;
        }
    }

    return nodeBInfo;
}

// /**
// FUNCTION   :: UmtsLayer3UeInitMm
// LAYER      :: Layer 3
// PURPOSE    :: Initialize UE RRC data
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + nodeInput : const NodeInput* : Pointer to node input.
// + ueL3      : UmtsLayer3Ue *   : Pointer to UMTS UE Layer3 data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeInitMm(Node *node,
                        const NodeInput *nodeInput,
                        UmtsLayer3Ue *ueL3)
{
    UmtsMmMsData* mmData;

    mmData = (UmtsMmMsData*)(MEM_malloc(sizeof(UmtsMmMsData)));
    memset(mmData, 0, sizeof(UmtsMmMsData));
    ueL3->mmData = mmData;

    mmData->mainState = UMTS_MM_MS_NULL;
    mmData->idleSubState = UMTS_MM_MS_IDLE_NULL;
    ueL3->mmData->updateState = UMTS_MM_NOT_UPDATED;

    mmData->actMmConns = new UmtsMmConnCont;
    mmData->rqtingMmConns = new UmtsMmConnQueue;
}

// /**
// FUNCTION   :: UmtsLayer3UeFinalizeMm
// LAYER      :: Layer 3
// PURPOSE    :: Initialize UE RRC data
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data*  : pointer to UMTS layer 3 data
// + ueL3      : UmtsLayer3Ue *   : Pointer to UMTS UE Layer3 data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeFinalizeMm(Node *node,
                            UmtsLayer3Data* umtsL3,
                            UmtsLayer3Ue *ueL3)
{
    UmtsMmMsData* ueMm = ueL3->mmData;

    UmtsClearStlContDeep(ueMm->actMmConns);
    delete ueMm->actMmConns;
    UmtsClearStlContDeep(ueMm->rqtingMmConns);
    delete ueMm->rqtingMmConns;

    if (ueMm->T3230)
    {
        MESSAGE_CancelSelfMsg(node, ueMm->T3230);
    }
    if (ueMm->T3240)
    {
        MESSAGE_CancelSelfMsg(node, ueMm->T3240);
    }

    MEM_free(ueL3->mmData);
    ueL3->mmData = NULL;
}

// /**
// FUNCTION   :: UmtsLayer3UeInitGMm
// LAYER      :: Layer 3
// PURPOSE    :: Initialize UE GMM data
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + nodeInput : const NodeInput* : Pointer to node input.
// + umtsL3    : UmtsLayer3Data*  : Pint to the umts L3 data
// + ueL3      : UmtsLayer3Ue *   : Pointer to UMTS UE Layer3 data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeInitGMm(Node *node,
                        const NodeInput *nodeInput,
                        UmtsLayer3Data* umtsL3,
                        UmtsLayer3Ue *ueL3)
{
    UmtsGMmMsData* gmmData;

    gmmData = (UmtsGMmMsData*)(MEM_malloc(sizeof(UmtsGMmMsData)));
    memset(gmmData, 0, sizeof(UmtsGMmMsData));
    ueL3->gmmData = gmmData;

    gmmData->updateState = UMTS_GMM_NOT_UPDATED;
    gmmData->mainState = UMTS_GMM_MS_NULL;
    gmmData->pmmState = UMTS_PMM_DETACHED;

    // TODO determine if the UE support GPRS
    if (1)
    {
        gmmData->mainState = UMTS_GMM_MS_DEREGISTERED;

        // attach needed assume attach when poweron/init phase
        // the sirst attach will triggered when loction update is send
        ueL3->gmmData->deRegSubState =
            UMTS_GMM_MS_DEREGISTERED_ATTACH_NEEDED;
        ueL3->gmmData->nextAttachType = UMTS_GPRS_ATTACH_ONLY_NO_FOLLOW_ON;
        ueL3->gmmData->attachAtmpCount = 0;
    }
}

// /**
// FUNCTION   :: UmtsLayer3UeFinalizeGMm
// LAYER      :: Layer 3
// PURPOSE    :: Initialize UE GMM data
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data*  : Pint to the umts L3 data
// + ueL3      : UmtsLayer3Ue *   : Pointer to UMTS UE Layer3 data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeFinalizeGMm(Node *node,
                        UmtsLayer3Data* umtsL3,
                        UmtsLayer3Ue *ueL3)
{
    if (ueL3->gmmData->T3310)
    {
        MESSAGE_CancelSelfMsg(node, ueL3->gmmData->T3310);
        ueL3->gmmData->T3310 = NULL;
    }
    if (ueL3->gmmData->T3317)
    {
        MESSAGE_CancelSelfMsg(node, ueL3->gmmData->T3317);
        ueL3->gmmData->T3317 = NULL;
    }
    if (ueL3->gmmData->T3319)
    {
        MESSAGE_CancelSelfMsg(node, ueL3->gmmData->T3319);
        ueL3->gmmData->T3319 = NULL;
    }
    MEM_free(ueL3->gmmData);
    ueL3->gmmData = NULL;
}

// /**
// FUNCTION   :: UmtsLayer3UeStopCcTimers
// LAYER      :: Layer 3
// PURPOSE    :: Stop active CC timers
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + ueL3      : UmtsLayer3Ue *   : Pointer to UMTS UE Layer3 data
// + ueCc      : UmtsCcMsData*    : Pointer to the CC entity data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeStopCcTimers(Node* node,
                              UmtsLayer3Ue* ueL3,
                              UmtsCcMsData* ueCc)
{
    if (ueCc->T303)
    {
        MESSAGE_CancelSelfMsg(node, ueCc->T303);
        ueCc->T303 = NULL;
    }
    if (ueCc->T310)
    {
        MESSAGE_CancelSelfMsg(node, ueCc->T310);
        ueCc->T310 = NULL;
    }
    if (ueCc->T313)
    {
        MESSAGE_CancelSelfMsg(node, ueCc->T313);
        ueCc->T313 = NULL;
    }
}

// /**
// FUNCTION   :: UmtsLayer3UeInitCc
// LAYER      :: Layer 3
// PURPOSE    :: Initialize UE Call Control entity
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + ueL3      : UmtsLayer3Ue *   : Pointer to UMTS UE Layer3 data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeInitCc(Node *node,
                        UmtsLayer3Ue *ueL3,
                        UmtsLayer3UeAppInfo* ueApp)
{
    UmtsCcMsData* ueCc = (UmtsCcMsData*)MEM_malloc(sizeof(UmtsCcMsData));
    memset(ueCc, 0, sizeof(UmtsCcMsData));
    ueCc->state = UMTS_CC_MS_NULL;

    ueApp->cmData.ccData = ueCc;
}

// /**
// FUNCTION   :: UmtsLayer3UeFinalizeCc
// LAYER      :: Layer 3
// PURPOSE    :: Finnalize UE Call Control entity
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + ueL3      : UmtsLayer3Ue *   : Pointer to UMTS UE Layer3 data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeFinalizeCc(Node *node,
                            UmtsLayer3Ue *ueL3,
                            UmtsLayer3UeAppInfo* ueApp)
{
    UmtsCcMsData* ueCc = ueApp->cmData.ccData;
    UmtsLayer3UeStopCcTimers(node, ueL3, ueCc);
    if (ueCc->T305)
    {
        MESSAGE_CancelSelfMsg(node, ueCc->T305);
        ueCc->T305 = NULL;
    }
    if (ueCc->T308)
    {
        MESSAGE_CancelSelfMsg(node, ueCc->T308);
        ueCc->T308 = NULL;
    }
    MEM_free(ueCc);
    ueApp->cmData.ccData = NULL;
}
// /**
// FUNCTION   :: UmtsLayer3UeUpdateStatAfterFailure
// LAYER      :: Layer3
// PURPOSE    :: Called by RLC to report unrecoverable
//               error for AM RLC entity.
// PARAMETERS ::
// + node      : Node*             : Pointer to node.
// + umtsL3    : UmtsLayer3Data *  : Pointer to UMTS Layer3 data
static
void UmtsLayer3UeUpdateStatAfterFailure(Node* node,
                                        UmtsLayer3Data* umtsL3,
                                        UmtsLayer3Ue* ueL3)
{
    UmtsLayer3UeAppInfo* appInfo = NULL;

    std::list<UmtsLayer3UeAppInfo*>::iterator it;
    //UmtsLayer3CnDomainId domainInfo;

    for (it = ueL3->appInfoList->begin();
        it != ueL3->appInfoList->end();
        it ++)
    {
        appInfo = (*it);
        if (appInfo->classifierInfo->domainInfo ==
            UMTS_LAYER3_CN_DOMAIN_CS)
        {
            continue;
        }

        if (appInfo->cmData.smData->smState != UMTS_SM_MS_PDP_ACTIVE)
        {
            // only those active one can be consider as dropped.
            continue;
        }
        if (appInfo->srcDestType == UMTS_FLOW_NETWORK_ORIGINATED)
        {
            ueL3->stat.numMtAppDropped ++;
            if (appInfo->qosInfo->trafficClass ==
                    UMTS_QOS_CLASS_CONVERSATIONAL)
            {
                ueL3->stat.numMtConversationalAppDropped ++;
            }
            else if (appInfo->qosInfo->trafficClass ==
                UMTS_QOS_CLASS_STREAMING)
            {
                ueL3->stat.numMtStreamingAppDropped ++;
            }
            else if (appInfo->qosInfo->trafficClass ==
                UMTS_QOS_CLASS_BACKGROUND)
            {
                ueL3->stat.numMtBackgroundAppDropped ++;
            }
            else if (appInfo->qosInfo->trafficClass >
                UMTS_QOS_CLASS_STREAMING &&
                appInfo->qosInfo->trafficClass <
                UMTS_QOS_CLASS_BACKGROUND)
            {
                ueL3->stat.numMtInteractiveAppDropped ++;
            }
        }
        else if (appInfo->srcDestType == UMTS_FLOW_MOBILE_ORIGINATED)
        {
            ueL3->stat.numMoAppDropped ++;
            if (appInfo->qosInfo->trafficClass ==
                    UMTS_QOS_CLASS_CONVERSATIONAL)
            {
                ueL3->stat.numMoConversationalAppDropped ++;
            }
            else if (appInfo->qosInfo->trafficClass ==
                UMTS_QOS_CLASS_STREAMING)
            {
                ueL3->stat.numMoStreamingAppDropped ++;
            }
            else if (appInfo->qosInfo->trafficClass ==
                UMTS_QOS_CLASS_BACKGROUND)
            {
                ueL3->stat.numMoBackgroundAppDropped ++;
            }
            else if (appInfo->qosInfo->trafficClass >
                UMTS_QOS_CLASS_STREAMING &&
                appInfo->qosInfo->trafficClass <
                UMTS_QOS_CLASS_BACKGROUND)
            {
                ueL3->stat.numMoInteractiveAppDropped ++;
            }
        } // if (appInfo->cmData.smData->smState
    } // for (it)
}

// /**
// FUNCTION   :: UmtsLayer3UeAddAppInfoToList
// LAYER      :: Layer 3
// PURPOSE    :: Add a new Flow Info to the AppList
// + node      : Node*             : Pointer to node.
// + ueL3      : UmtsLayer3Ue*     : Pointer to the umts ue L3 data
// + srcDestType : UmtsFlowSrcDestType  : applicaiton src dest type
// + domainInfo: UmtsLayer3CnDomainId: The flow is PS or CS
// + transId   : unsigned char      : transactionId
// + appClsPtr : UmtsLayer3FlowClassifier* : Flow classfier
// RETURN     :: UmtsLayer3UeAppInfo* : newly added appInfo
// **/
static
UmtsLayer3UeAppInfo* UmtsLayer3UeAddAppInfoToList(
                          Node* node,
                          UmtsLayer3Ue* ueL3,
                          UmtsFlowSrcDestType srcDestType,
                          UmtsLayer3CnDomainId domainInfo,
                          unsigned char transId,
                          UmtsLayer3FlowClassifier* appClsPt = NULL)
{
    UmtsLayer3UeAppInfo* appInfo = NULL;
    int pos = 0;

    // assign a NSAPI
    if (domainInfo == UMTS_LAYER3_CN_DOMAIN_PS)
    {
        // only PS needs to set bit
        pos = UmtsGetPosForNextZeroBit<UMTS_MAX_NASPI>
                                      (*(ueL3->nsapiBitSet), 0);
        if (pos == -1)
        {
            ERROR_ReportWarning("reach max NSAPI");

            return appInfo;
        }
    }
    appInfo = new UmtsLayer3UeAppInfo(transId, srcDestType);

    // incldue the classifier info if it is UE initiated
    appInfo->classifierInfo = (UmtsLayer3FlowClassifier*)
                         MEM_malloc(sizeof(UmtsLayer3FlowClassifier));
    if (appClsPt)
    {
        memcpy(appInfo->classifierInfo,
               appClsPt,
               sizeof(UmtsLayer3FlowClassifier));
    }
    else
    {
        memset(appInfo->classifierInfo,
               0,
               sizeof(UmtsLayer3FlowClassifier));
        appInfo->classifierInfo->domainInfo = domainInfo;
    }

    if (domainInfo == UMTS_LAYER3_CN_DOMAIN_CS)
    {
        UmtsLayer3UeInitCc(node, ueL3, appInfo);
    }
    else
    {
        appInfo->cmData.smData =
            (UmtsSmMsData*) MEM_malloc(sizeof(UmtsSmMsData));
        memset((void*)(appInfo->cmData.smData), 0, sizeof(UmtsSmMsData));
        appInfo->cmData.smData->smState = UMTS_SM_MS_PDP_INACTIVE;
        appInfo->nsapi = (unsigned char)pos;
    }

    // QoS info
    appInfo->qosInfo =
        (UmtsFlowQoSInfo*)MEM_malloc(sizeof(UmtsFlowQoSInfo));
    memset((void*)(appInfo->qosInfo), 0, sizeof(UmtsFlowQoSInfo));

    // put into the list
    ueL3->appInfoList->push_back(appInfo);

    return appInfo;
}

// /**
// FUNCTION   :: UmtsLayer3UeAppInfoFinalize
// LAYER      :: Layer 3
// PURPOSE    :: Finalize a Flow info item in the appList
// + node      : Node*             : Pointer to node.
// + ueL3      : UmtsLayer3Ue*     : Pointer to the umts ue L3 data
// + appInfo   : UmtsLayer3UeAppInfo* : Pointer to the UE app info
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeAppInfoFinalize(Node* node,
                                 UmtsLayer3Ue* ueL3,
                                 UmtsLayer3UeAppInfo* appInfo)
{
    UmtsLayer3CnDomainId domainInfo = appInfo->classifierInfo->domainInfo;
    // reset bit set
    if (appInfo->srcDestType == UMTS_FLOW_MOBILE_ORIGINATED)
    {
        ueL3->transIdBitSet->reset(appInfo->transId);
    }
    if (appInfo->classifierInfo->domainInfo == UMTS_LAYER3_CN_DOMAIN_PS)
    {
        ueL3->nsapiBitSet->reset(appInfo->nsapi);
    }
    if (appInfo->classifierInfo)
    {
        MEM_free(appInfo->classifierInfo);
    }
    if (appInfo->cmData.ccData)
    {
        UmtsLayer3UeFinalizeCc(node, ueL3, appInfo);
    }
    if (appInfo->cmData.smData)
    {
        if (appInfo->cmData.smData->T3380)
        {
            MESSAGE_CancelSelfMsg(
                node,
                appInfo->cmData.smData->T3380);
            appInfo->cmData.smData->T3380 = NULL;
        }
        if (appInfo->cmData.smData->T3390)
        {
            MESSAGE_CancelSelfMsg(
                node,
                appInfo->cmData.smData->T3390);
            appInfo->cmData.smData->T3390 = NULL;
        }

        MEM_free(appInfo->cmData.smData);
        appInfo->cmData.smData = NULL;
    }

    // free QoS
    if (appInfo->qosInfo)
    {
        MEM_free(appInfo->qosInfo);
    }
    appInfo->classifierInfo = NULL;

    // free pakcet in buffer
    while (!appInfo->inPacketBuf->empty())
    {
        MESSAGE_Free(node,
                     appInfo->inPacketBuf->front());
        appInfo->inPacketBuf->pop();
        if (domainInfo == UMTS_LAYER3_CN_DOMAIN_PS)
        {
            ueL3->stat.numPsDataPacketFromUpperDeQueue ++;
            ueL3->stat.numPsPktDroppedNoResource ++;
        }
        else if (domainInfo == UMTS_LAYER3_CN_DOMAIN_CS)
        {
            ueL3->stat.numCsDataPacketFromUpperDeQueue ++;
            ueL3->stat.numCsPktDroppedNoResource ++;
        }
    }
    appInfo->pktBufSize = 0;

    appInfo->classifierInfo = NULL;
    delete appInfo;
}

// /**
// FUNCTION   :: UmtsLayer3UeRemoveAppInfoFromList
// LAYER      :: Layer 3
// PURPOSE    :: Remove a Flow info item from the appList
// + node      : Node*             : Pointer to node.
// + ueL3      : UmtsLayer3Ue*     : Pointer to the umts ue L3 data
// + srcDestType : UmtsFlowSrcDestType  : applicaiton src dest type
// + transId   : unsigned char      : transactionId
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeRemoveAppInfoFromList(Node* node,
                                       UmtsLayer3Ue* ueL3,
                                       UmtsFlowSrcDestType srcDestType,
                                       unsigned char transId)
{
    std::list<UmtsLayer3UeAppInfo*>::iterator it;


    for (it = ueL3->appInfoList->begin();
        it != ueL3->appInfoList->end();
        it ++)
    {
        if ((*it) && (*it)->transId == transId &&
            (*it)->srcDestType == srcDestType)
        {
            break;
        }
    }

    ERROR_Assert(it != ueL3->appInfoList->end(),
                 " No appInfo in the list link "
                 "with the transId and srcDestType");

    UmtsLayer3UeAppInfoFinalize(node,
                                ueL3,
                                (*it));
    ueL3->appInfoList->erase(it);
}

// /**
// FUNCTION   :: UmtsLayer3UeFindAppInfoFromList
// LAYER      :: Layer 3
// PURPOSE    :: Look for the app Info from the appList
// + node      : Node*             : Pointer to node.
// + ueL3      : UmtsLayer3Ue*     : Pointer to the umts ue L3 data
// + srcDestType : UmtsFlowSrcDestType  : applicaiton src dest type
// + transId   : unsigned char      : transactionId
// RETURN     :: UmtsLayer3UeAppInfo* : found appInfo
// **/
static
UmtsLayer3UeAppInfo* UmtsLayer3UeFindAppInfoFromList(Node* node,
                                     UmtsLayer3Ue* ueL3,
                                     UmtsFlowSrcDestType srcDestType,
                                     unsigned char transId)
{
    std::list<UmtsLayer3UeAppInfo*>::iterator it;

    for (it = ueL3->appInfoList->begin();
        it != ueL3->appInfoList->end();
        it ++)
    {
        if ((*it)->transId == transId &&
            (*it)->srcDestType == srcDestType)
        {
            break;
        }
    }

    if (it != ueL3->appInfoList->end())
    {
        return (*it);
    }
    else
    {
        return (UmtsLayer3UeAppInfo*)NULL;
    }
}

// /**
// FUNCTION   :: UmtsLayer3UeFindAppInfoFromList
// LAYER      :: Layer 3
// PURPOSE    :: Look for the app Info from the appList
// + node      : Node*             : Pointer to node.
// + ueL3      : UmtsLayer3Ue*     : Pointer to the umts ue L3 data
// + transId   : unsigned char      : transactionId
// RETURN     :: UmtsLayer3UeAppInfo* : found appInfo
// **/
static
UmtsLayer3UeAppInfo* UmtsLayer3UeFindAppInfoFromList(
                        Node* node,
                        UmtsLayer3Ue* ueL3,
                        char transId)
{
    std::list<UmtsLayer3UeAppInfo*>::iterator it;

    for (it = ueL3->appInfoList->begin();
        it != ueL3->appInfoList->end();
        it ++)
    {
        if ((*it)->transId == transId)
        {
            break;
        }
    }

    if (it != ueL3->appInfoList->end())
    {
        return (*it);
    }
    else
    {
        return (UmtsLayer3UeAppInfo*)NULL;
    }
}

// /**
// FUNCTION   :: UmtsLayer3UeFindAppInfoFromList
// LAYER      :: Layer 3
// PURPOSE    :: Configurate the CCCH, RACH and PRACH for random access
// + node      : Node*             : Pointer to node.
// + ueL3      : UmtsLayer3Ue*     : Pointer to the umts ue L3 data
// + appClsPt  : UmtsLayer3FlowClassifier* : pointer to the classfier info
// RETURN     :: UmtsLayer3UeAppInfo* : found appInfo
// **/
static
UmtsLayer3UeAppInfo* UmtsLayer3UeFindAppInfoFromList(Node* node,
                                     UmtsLayer3Ue* ueL3,
                                     UmtsLayer3FlowClassifier* appClsPt)
{
    std::list<UmtsLayer3UeAppInfo*>::iterator it;

    for (it = ueL3->appInfoList->begin();
        it != ueL3->appInfoList->end();
        it ++)
    {

        if (memcmp((void*)(((*it)->classifierInfo)),
                   (void*)(appClsPt),
                   sizeof(UmtsLayer3FlowClassifier)) == 0)
        {
            break;
        }
    }

    if (it != ueL3->appInfoList->end())
    {
        return (*it);
    }
    else
    {
        return (UmtsLayer3UeAppInfo*)NULL;
    }
}

// /**
// FUNCTION   :: UmtsLayer3UeFindAppByRabId
// LAYER      :: Layer 3
// PURPOSE    :: Find a UE flow by RAB ID
// + node      : Node*             : Pointer to node.
// + ueL3      : UmtsLayer3Ue*     : Pointer to the umts ue L3 data
// + rabId     : char              : RAB ID
// RETURN     :: UmtsLayer3UeAppInfo* : found appInfo
// **/
static
UmtsLayer3UeAppInfo* UmtsLayer3UeFindAppByRabId(
                            Node* node,
                            UmtsLayer3Ue* ueL3,
                            char rabId)
{
    std::list<UmtsLayer3UeAppInfo*>::iterator ueFlowIter;
    ueFlowIter = find_if (ueL3->appInfoList->begin(),
                         ueL3->appInfoList->end(),
                         UmtsFindItemByRabId<UmtsLayer3UeAppInfo>(rabId));
    if (ueFlowIter != ueL3->appInfoList->end())
    {
        return *ueFlowIter;
    }
    else
    {
        return NULL;
    }
}

// /**
// FUNCTION   :: UmtsLayer3UeEnquePsPktFromUpper
// LAYER      :: Layer 3
// PURPOSE    :: Enqueue PS packet from upper lyer
// + node      : Node*             : Pointer to node.
// + ueL3      : UmtsLayer3Ue*     : Pointer to the umts ue L3 data
// + appInfo   : UmtsLayer3UeAppInfo* : application info
// + msg       : Message*          : messge to be queued
// RETURN     :: UmtsLayer3UeAppInfo* : found appInfo
// **/
static
void UmtsLayer3UeEnquePsPktFromUpper(
        Node* node,
        UmtsLayer3Ue* ueL3,
        UmtsLayer3UeAppInfo* appInfo,
        Message* msg)
{
    if (appInfo->pktBufSize + MESSAGE_ReturnPacketSize(msg)
            < UMTS_UE_MAXBUFSIZE_PERFLOW )
    {
        appInfo->inPacketBuf->push(msg);
        ueL3->stat.numPsDataPacketFromUpperEnQueue ++;
        appInfo->pktBufSize += MESSAGE_ReturnPacketSize(msg);
    }
    else
    {
        if (appInfo->qosInfo->trafficClass >= UMTS_QOS_CLASS_STREAMING)
        {
            MESSAGE_Free(node, msg);
            ueL3->stat.numPsPktDroppedNoResource++;
        }
        else
        {
            appInfo->inPacketBuf->push(msg);
            ueL3->stat.numPsDataPacketFromUpperEnQueue++;
            appInfo->pktBufSize += MESSAGE_ReturnPacketSize(msg);
            while (appInfo->pktBufSize > UMTS_UE_MAXBUFSIZE_PERFLOW)
            {
                Message* toFree = appInfo->inPacketBuf->front();
                appInfo->pktBufSize -= MESSAGE_ReturnPacketSize(toFree);
                MESSAGE_Free(node, toFree);
                appInfo->inPacketBuf->pop();
                ueL3->stat.numPsDataPacketFromUpperEnQueue++;
            } // while
        } // if (appInfo->qosInfo->trafficClass)
    } // if (appInfo->pktBufSize)
}

// /**
// FUNCTION   :: UmtsLayer3UeFindUmtsCallApp
// LAYER      :: Layer 3
// PURPOSE    :: Look for the umts call app Info from the appList
// + node      : Node*             : Pointer to node.
// + ueL3      : UmtsLayer3Ue*     : Pointer to the umts ue L3 data
// + appErcNodeId : NodeId         : source nodeId of application
// + appId     : int               : appliction id
// RETURN     :: UmtsLayer3UeAppInfo* : found appInfo
// **/
static
UmtsLayer3UeAppInfo* UmtsLayer3UeFindUmtsCallApp(
                        Node* node,
                        UmtsLayer3Ue* ueL3,
                        NodeId appSrcNodeId,
                        int appId)
{
    std::list<UmtsLayer3UeAppInfo*>::iterator appIter;

    for (appIter = ueL3->appInfoList->begin();
         appIter != ueL3->appInfoList->end();
         ++appIter)
    {
        if ((*appIter)->srcDestType == UMTS_FLOW_MOBILE_ORIGINATED
            && (*appIter)->cmData.ccData)
        {
            if ((*appIter)->cmData.ccData->callAppId == appId
                && appSrcNodeId == node->nodeId)
            {
                break;
            }
        }
        else if ((*appIter)->srcDestType == UMTS_FLOW_NETWORK_ORIGINATED
                 && (*appIter)->cmData.ccData)
        {
            if ((*appIter)->cmData.ccData->callAppId == appId
                && (*appIter)->cmData.ccData->peerAddr == appSrcNodeId)
            {
                break;
            }
        }
    }

    if (appIter != ueL3->appInfoList->end())
    {
        return *appIter;
    }
    else
    {
        return NULL;
    }
}

// /**
// FUNCTION   :: UmtsGMmUeGetGMmState
// LAYER      :: Layer3 GMM
// PURPOSE    :: get GMM state
// PARAMETERS ::
// + ueL3             : UmtsLayer3Ue *    : Pointer to l3 Ue data
// RETURN     :: UmtsGmmMsState : ueL3->gmmData->mainState
// **/
static
UmtsGmmMsState UmtsGMmUeGetGMmState(UmtsLayer3Ue* ueL3)
{
    return (ueL3->gmmData->mainState);
}

// FUNCTION   :: UmtsGMmUeSetGMmState
// LAYER      :: Layer3 GMM
// PURPOSE    :: get GMM state
// PARAMETERS ::
// + ueL3             : UmtsLayer3Ue *    : Pointer to l3 Ue data
// + newState         : UmtsGmmMsState    : new state to set
// RETURN     :: void : NULL
// **/
static
void UmtsGMmUeSetGMmState(UmtsLayer3Ue* ueL3,
                          UmtsGmmMsState newState)
{
    ueL3->gmmData->mainState = newState;
}

// /**
// FUNCTION   :: UmtsLayer3UeReleaseRandomAccessChannel
// LAYER      :: Layer 3
// PURPOSE    :: release the CCCH, RACH and PRACH used for random access
// + node      : Node*             : Pointer to node.
// + umtsL3    : UmtsLayer3Data *  : Pointer to the umts layer3 data
// + ueL3      : UmtsLayer3Ue*     : Pointer to the umts ue L3 data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeReleaseRandomAccessChannel(Node* node,
                                            UmtsLayer3Data *umtsL3,
                                            UmtsLayer3Ue* ueL3)
{
    // release PRACH
    UmtsCphyRadioLinkRelReqInfo relReqInfo;

    relReqInfo.phChDir = UMTS_CH_UPLINK;
    relReqInfo.phChType = UMTS_PHYSICAL_PRACH;
    relReqInfo.phChId = 1;

    UmtsLayer3ReleaseRadioLink(node, umtsL3, &relReqInfo);

    // release RACH
    UmtsCphyTrChReleaseReqInfo trRelInfo;

    trRelInfo.trChDir = UMTS_CH_UPLINK;
    trRelInfo.trChId = 1;
    trRelInfo.trChType = UMTS_TRANSPORT_RACH;
    UmtsLayer3ReleaseTrCh(node, umtsL3, &trRelInfo);

    // release Radio beaer
    UmtsCmacConfigReqRbInfo rbInfo;

    rbInfo.chDir = UMTS_CH_UPLINK;
    rbInfo.logChId = 1;
    rbInfo.logChType = UMTS_LOGICAL_CCCH;
    rbInfo.logChPrio = 1;
    rbInfo.priSc = node->nodeId;
    rbInfo.rbId = 0;
    rbInfo.trChType = UMTS_TRANSPORT_RACH;
    rbInfo.ueId = node->nodeId;
    rbInfo.trChId = 1;
    rbInfo.releaseRb = TRUE;

    UmtsLayer3ReleaseRadioBearer(node, umtsL3, &rbInfo);

    //Release RLC entity for RACH
    UmtsLayer3ReleaseRlcEntity(node,
                               umtsL3,
                               rbInfo.rbId,
                               UMTS_RLC_ENTITY_TX);

    //Release RLC entity for FACH
    UmtsLayer3ReleaseRlcEntity(node,
                               umtsL3,
                               rbInfo.rbId,
                               UMTS_RLC_ENTITY_RX);

}

// /**
// FUNCTION   :: UmtsLayer3UeConfigRandomAccessChannel
// LAYER      :: Layer 3
// PURPOSE    :: Configurate the CCCH, RACH and PRACH for random access
// + node      : Node*             : Pointer to node.
// + umtsL3    : UmtsLayer3Data *  : Pointer to the umts layer3 data
// + ueL3      : UmtsLayer3Ue*     : Pointer to the umts ue L3 data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeConfigRandomAccessChannel(Node* node,
                                           UmtsLayer3Data *umtsL3,
                                           UmtsLayer3Ue* ueL3)
{
    // config the PRACH
    UmtsCphyRadioLinkSetupReqInfo rlInfo;
    UmtsCphyChDescPRACH prachDesc;
    UmtsLayer3UeNodebInfo* nodebInfo;

    rlInfo.phChDir = UMTS_CH_UPLINK;
    rlInfo.ueId = node->nodeId;
    rlInfo.phChType = UMTS_PHYSICAL_PRACH;
    rlInfo.phChId = 1;
    rlInfo.priSc = 0; // no meaning to PRACH

    nodebInfo = UmtsLayer3UeGetNodeBData(node,
                                         umtsL3,
                                         ueL3,
                                         ueL3->rrcData->selectedCellId);

    ERROR_Assert(nodebInfo, "no nodeBInfo link with the selected cell");

    prachDesc.accessSlot = 2; // TODO, it will generate at run itme

    int temp = (int)(RANDOM_erand(umtsL3->seed) * 16);

    // random select from the 0-15 in that group
    prachDesc.preScCode = ueL3->priNodeB->primaryScCode + temp;

    prachDesc.availPreSigniture = node->nodeId; // no meaning at this layer
    prachDesc.dataSpreadFactor = UMTS_SF_256;
                       // assume 256 is used for PRACH data and control part

    prachDesc.ascAvailPreSigniture = node->nodeId;
                      // we assume all are available
    prachDesc.ascAvailSubCh = 1; // assume all for the ASC are avaible

    // set the power control part
    prachDesc.initTxPower = (PHY_UMTS_MAX_TX_POWER_dBm +
                             PHY_UMTS_MAX_TX_POWER_dBm)/2;
    prachDesc.powerOffset = 0; // assuem no offset between data/ctrl part
    prachDesc.powerRampStep = 6;

    rlInfo.phChDesc = (void*)&prachDesc;

    UmtsMacPhyHandleInterLayerCommand(node,
        umtsL3->interfaceIndex,
        UMTS_CPHY_RL_SETUP_REQ,
        (void*)&rlInfo);
    if (DEBUG_RRC_RL)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n\t Configure a PRACH with preamsble code %d sig %d\n",
            prachDesc.preScCode, prachDesc.availPreSigniture);
    }

    // config the RACH transport channel
    UmtsCphyTrChConfigReqInfo trCfgInfo;
    trCfgInfo.trChDir = UMTS_CH_UPLINK;
    trCfgInfo.trChId = 1;
    trCfgInfo.trChType = UMTS_TRANSPORT_RACH;
    trCfgInfo.ueId = node->nodeId;
    trCfgInfo.logChPrio = 0; // al lcontrol channel use 0

    // use default RACH/PRACH tranpsort format
    memcpy((void*)&(trCfgInfo.transFormat),
       (void*)&umtsDefaultRachTransFormat,
       sizeof(UmtsTransportFormat));

    UmtsMacPhyHandleInterLayerCommand(
        node,
        umtsL3->interfaceIndex,
        UMTS_CPHY_TrCH_CONFIG_REQ,
        (void*)&trCfgInfo);

    if (DEBUG_RRC_TRCH)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n\tConfigure a RACH TrCh\n");
    }

    // config RACH info
    UmtsCmacConfigReqRachInfo rachCfgInfo;

    // configurate rach with info gotten from BCH
    rachCfgInfo.releaseRachInfo = FALSE;
    rachCfgInfo.Mmax = 7;
    rachCfgInfo.NB01min = 1;
    rachCfgInfo.NB01max = 10;
    rachCfgInfo.ascIndex = 0; // this is the one select
                              // by RRC based on priority
    rachCfgInfo.prachPartition = new std::list<UmtsRachAscInfo*>;

    // populate the list
    UmtsRachAscInfo ascInfo;
    ascInfo.ascIndex = 0; // TODO highest 0 lowest 7
    ascInfo.assignedSubCh = 0x07; // TODO, only subchannel 0,1,2
                                  // is assigned right now
    ascInfo.persistFactor = 1;
    ascInfo.sigStartIndex = 0;
    ascInfo.sigEndIndex = 15; // assume all sig are avaible for asc 0

    rachCfgInfo.prachPartition->push_back(&ascInfo);

    UmtsMacHandleInterLayerCommand(node,
        umtsL3->interfaceIndex,
        UMTS_CMAC_CONFIG_REQ_RACH,
        (void*)&rachCfgInfo);

    delete rachCfgInfo.prachPartition;

    // config radio bearer and mapping
    // CCCH-RACH
    UmtsCmacConfigReqRbInfo rbInfo;
    UmtsTrCh2PhChMappingInfo mapInfo;

    rbInfo.chDir = UMTS_CH_UPLINK;
    rbInfo.logChId = 1;
    rbInfo.logChType = UMTS_LOGICAL_CCCH;
    rbInfo.logChPrio = 1;
    rbInfo.priSc = node->nodeId;
    rbInfo.rbId = UMTS_CCCH_RB_ID;
    rbInfo.trChType = UMTS_TRANSPORT_RACH;
    rbInfo.ueId = node->nodeId;
    rbInfo.trChId = 1;
    rbInfo.releaseRb = FALSE;
    UmtsMacHandleInterLayerCommand(node,
        umtsL3->interfaceIndex,
        UMTS_CMAC_CONFIG_REQ_RB,
        (void*)&rbInfo);

    mapInfo.rbId = rbInfo.rbId;
    mapInfo.chDir = UMTS_CH_UPLINK;
    mapInfo.priSc = node->nodeId; // TODO
    mapInfo.ueId = node->nodeId;
    mapInfo.trChType = UMTS_TRANSPORT_RACH;
    mapInfo.trChId = 1;
    mapInfo.phChType = UMTS_PHYSICAL_PRACH;
    mapInfo.phChId = 1;

    // use default RACH/PRACH tranpsort format
    memcpy((void*)&(mapInfo.transFormat),
       (void*)&umtsDefaultRachTransFormat,
       sizeof(UmtsTransportFormat));
    UmtsMacPhyMappingTrChPhCh(node, umtsL3->interfaceIndex, &mapInfo);

    //Configure RLC Entity for RACH
    UmtsRlcTmConfig tmConfig;
    tmConfig.segIndi = FALSE;
    UmtsLayer3ConfigRlcEntity(
        node,
        umtsL3,
        rbInfo.rbId,
        UMTS_RLC_ENTITY_TX,
        UMTS_RLC_ENTITY_TM,
        &tmConfig,
        node->nodeId);

    // CCCH-FACH (AM)
    rbInfo.chDir = UMTS_CH_DOWNLINK;
    rbInfo.logChId = 1;
    rbInfo.logChType = UMTS_LOGICAL_CCCH;
    rbInfo.logChPrio = 0;
    rbInfo.priSc = node->nodeId;
    rbInfo.rbId = UMTS_CCCH_RB_ID;
    rbInfo.trChType = UMTS_TRANSPORT_FACH;
    rbInfo.ueId = node->nodeId;
    rbInfo.trChId = 1;
    rbInfo.releaseRb = FALSE;
    UmtsMacHandleInterLayerCommand(node,
        umtsL3->interfaceIndex,
        UMTS_CMAC_CONFIG_REQ_RB,
        (void*)&rbInfo);

    //Config RLC for FACH
    UmtsLayer3ConfigRlcEntity(
        node,
        umtsL3,
        rbInfo.rbId,
        UMTS_RLC_ENTITY_RX,
        UMTS_RLC_ENTITY_TM,
        &tmConfig,
        node->nodeId);

    if (DEBUG_RRC_RB)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n\tConfigure a UL/DL CCCH rb\n");
    }

    // start the RACH procedure by send a DATA_REQ to MAC
    // this MAC_DATA_REQ command only used for init rach procedure

    UmtsMacHandleInterLayerCommand(node,
        umtsL3->interfaceIndex,
        UMTS_MAC_DATA_REQ,
        (void*)NULL);
}

// /**
// FUNCTION   :: UmtsLayer3UeConfigCommonRadioBearer
// LAYER      :: Layer 3
// PURPOSE    :: Configurate the common Transport Channels
// PARAMETERS ::
// + node      : Node*             : Pointer to node.
// + umtsL3    : UmtsLayer3Data *  : Point to the umts layer3 data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeConfigCommonRadioBearer(Node *node,
                                         UmtsLayer3Data *umtsL3)
{
    // configure the common radio bearer
    UmtsCmacConfigReqRbInfo rbInfo;
    UmtsTrCh2PhChMappingInfo mapInfo;
    UmtsRlcTmConfig tmConfig;

    // BCCH Logical Channel
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

    tmConfig.segIndi = FALSE;
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
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n\tonfigure a BCCH rb\n");
    }

    // PCCH
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
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n\tConfigure a PCCH rb\n");
    }
}

#if 0
// /**
// FUNCTION   :: UmtsLayer3UeReleaseSignalRb1To3
// LAYER      :: Layer 3
// PURPOSE    :: Release signalling RB 1-3
// + node      : Node*             : Pointer to node.
// + umtsL3    : UmtsLayer3Data *  : Pointer to the umts layer3 data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeReleaseSignalRb1To3(
         Node* node,
         UmtsLayer3Data *umtsL3)
{
    UmtsLayer3Ue *ueL3 = (UmtsLayer3Ue *) (umtsL3->dataPtr);

    // release Physical channel(Uplink)
    UmtsCphyRadioLinkRelReqInfo relReqInfo;
    relReqInfo.phChDir = UMTS_CH_UPLINK;
    relReqInfo.phChType = UMTS_PHYSICAL_DPDCH;
    relReqInfo.phChId = UMTS_SRB_DPDCH_ID;
    UmtsLayer3ReleaseRadioLink(node, umtsL3, &relReqInfo);

    // release transport channel(Uplink)
    UmtsCphyTrChReleaseReqInfo trRelInfo;
    trRelInfo.trChDir = UMTS_CH_UPLINK;
    trRelInfo.trChId = UMTS_SRB_DCH_ID;
    trRelInfo.trChType = UMTS_TRANSPORT_DCH;
    UmtsLayer3ReleaseTrCh(node, umtsL3, &trRelInfo);


    // release Radio beaer
    // Rlease upLink bearer
    for (int i = 0; i < 3; i++)
    {
        UmtsCmacConfigReqRbInfo rbInfo;
        rbInfo.chDir = UMTS_CH_UPLINK;
        rbInfo.logChId = (unsigned char)i ;
        rbInfo.logChType = UMTS_LOGICAL_DCCH;
        rbInfo.logChPrio = (unsigned char)(i+1);
        rbInfo.priSc = ueL3->rrcData->ueScCode;
        rbInfo.rbId = (unsigned char)(i+1);
        rbInfo.trChType = trRelInfo.trChType;
        rbInfo.ueId = node->nodeId;
        rbInfo.trChId = trRelInfo.trChId;
        rbInfo.releaseRb = TRUE;
        UmtsLayer3ReleaseRadioBearer(node, umtsL3, &rbInfo);

        //Release uplink entity
        UmtsLayer3ReleaseRlcEntity(node,
                                   umtsL3,
                                   (unsigned char)(i+1),
                                   UMTS_RLC_ENTITY_BI);
    }

    // release DPCCH
    relReqInfo.phChDir = UMTS_CH_UPLINK;
    relReqInfo.phChType = UMTS_PHYSICAL_DPCCH;
    relReqInfo.phChId = 1;
    UmtsLayer3ReleaseRadioLink(node, umtsL3, &relReqInfo);
}
#endif

// /**
// FUNCTION   :: UmtsLayer3UeConfigSignalRb1To3
// LAYER      :: Layer 3
// PURPOSE    :: Setup signalling RB 1-3
// + node      : Node*                    : Pointer to node.
// + umtsL3    : UmtsLayer3Data *         : Pointer to the umts layer3 data
// + ueScCode  : unsigned int             : UE primary scrambling code
// + dlDpdchInfo: const UmtsRcvPhChInfo*  : downlink DPDCH information
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeConfigSignalRb1To3(
    Node* node,
    UmtsLayer3Data *umtsL3,
    unsigned int ueScCode,
    const UmtsRcvPhChInfo* dlDpdchInfo)
{
    UmtsLayer3Ue *ueL3 = (UmtsLayer3Ue *) (umtsL3->dataPtr);
    ueL3->rrcData->ueScCode = ueScCode;

    //Configure the physical channel(Uplink)
    UmtsCphyRadioLinkSetupReqInfo rlInfo;
    UmtsCphyChDescUlDPCH phChDesc;
    UmtsTransportFormat transFormat;

    // assume 3.4k Singalling
    memcpy((void*)&transFormat,
           (void*)&umtsDefaultSigDchTransFormat_3400,
           sizeof(UmtsTransportFormat));

    // config the spread factor of PHY CHs
    UmtsUeSetSpreadFactorInUse(
        node, umtsL3->interfaceIndex, UMTS_SF_256);

    phChDesc.ulScCode = ueScCode;

    UmtsChannelDirection direction = UMTS_CH_UPLINK;
    rlInfo.phChDir = direction;
    rlInfo.ueId = node->nodeId;
    rlInfo.phChType = UMTS_PHYSICAL_DPDCH;
    rlInfo.phChId = UMTS_SRB_DPDCH_ID;
    rlInfo.priSc = ueScCode;
    rlInfo.phChDesc = &phChDesc;
    UmtsLayer3ConfigRadioLink(node, umtsL3, &rlInfo);

    // Configure the transport channel(Uplink)
    UmtsCphyTrChConfigReqInfo trCfgInfo;
    trCfgInfo.trChDir = direction;
    trCfgInfo.trChId = UMTS_SRB_DCH_ID;
    trCfgInfo.trChType = UMTS_TRANSPORT_DCH;
    trCfgInfo.ueId = node->nodeId;
    trCfgInfo.logChPrio = 0; // all signalling use 0
    memcpy((void*)&trCfgInfo.transFormat,
           (void*)&transFormat,
           sizeof(UmtsTransportFormat));
    UmtsLayer3ConfigTrCh(node, umtsL3, &trCfgInfo);

    //configure the downlink DPDCH to only receive
    //packets received on the channel with assigned OVSF code
    UmtsMacPhyConfigRcvPhCh(node,
                            umtsL3->interfaceIndex,
                            ueL3->rrcData->selectedCellId,
                            dlDpdchInfo);

    // Config radio bearer and make mapping
    for (int i = 0; i < 3; i++)
    {
        UmtsCmacConfigReqRbInfo rbInfo;
        UmtsTrCh2PhChMappingInfo mapInfo;

        // Configure Uplink
        rbInfo.chDir = UMTS_CH_UPLINK;
        rbInfo.logChId = (unsigned char)i;
        rbInfo.logChType = UMTS_LOGICAL_DCCH;
        rbInfo.logChPrio = (unsigned char)(i+1);
        rbInfo.priSc = ueScCode;
        rbInfo.rbId = (unsigned char)(i+1);
        rbInfo.trChType = trCfgInfo.trChType;
        rbInfo.ueId = node->nodeId;
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

        //Configure DownLink
        rbInfo.chDir = UMTS_CH_DOWNLINK;
        rbInfo.logChId = (unsigned char)i;
        rbInfo.logChType = UMTS_LOGICAL_DCCH;
        rbInfo.logChPrio = (unsigned char)(i+1);
        rbInfo.priSc = ueScCode;
        rbInfo.rbId = (unsigned char)(i+1);
        rbInfo.trChType = trCfgInfo.trChType;
        rbInfo.ueId = node->nodeId;
        rbInfo.trChId = trCfgInfo.trChId;
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
            //Configure upLink Entity
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
                rbInfo.ueId);

            //Config downLink Entity
            UmtsLayer3ConfigRlcEntity(
                node,
                umtsL3,
                rbInfo.rbId,
                UMTS_RLC_ENTITY_RX,
                UMTS_RLC_ENTITY_UM,
                &entityConfig,
                rbInfo.ueId);
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
                rbInfo.ueId);
        }
    }

    // configurate the DPCCH
    rlInfo.phChType = UMTS_PHYSICAL_DPCCH;
    rlInfo.phChId = 1;

    UmtsMacPhyHandleInterLayerCommand(node,
        umtsL3->interfaceIndex,
        UMTS_CPHY_RL_SETUP_REQ,
        (void*)&rlInfo);
    if (DEBUG_RRC_RL)
    {
        printf("node %d (L3): configure a DPCCH\n", node->nodeId);
    }
}

// /**
// FUNCTION   :: UmtsLayer3UeReleaseRsrcByRbId
// LAYER      :: Layer3 RRC
// PURPOSE    :: Release low layer resources by RB ID
// PARAMETERS ::
// + node       : Node*                 : Pointer to node.
// + umtsL3     : UmtsLayer3Data *      : Pointer to UMTS Layer3 data
// + ueL3       : UmtsLayer3Ue*         : Ue data
// + rbId       : char                  : radio bearer Id
// RETURN     :: NULL
// **/
static
void UmtsLayer3UeReleaseRsrcByRbId(
    Node* node,
    UmtsLayer3Data* umtsL3,
    UmtsLayer3Ue* ueL3,
    char rbId)
{
    UmtsCmacConfigReqRbInfo rbInfo;
    memset(&rbInfo, 0, sizeof(rbInfo));
    rbInfo.chDir = UMTS_CH_UPLINK;
    rbInfo.rbId = rbId;
    rbInfo.ueId = node->nodeId;
    rbInfo.releaseRb = TRUE;
    UmtsLayer3ReleaseRadioBearer(node, umtsL3, &rbInfo);

    UmtsLayer3ReleaseRlcEntity(node,
                               umtsL3,
                               rbId,
                               UMTS_RLC_ENTITY_BI);
}

// /**
// FUNCTION   :: UmtsLayer3UeReleaseRcvPh
// LAYER      :: Layer 3
// PURPOSE    :: Release downlink decdicated physical channel listening to
// + node      : Node*             : Pointer to node.
// + umtsL3    : UmtsLayer3Data *  : Pointer to the umts layer3 data
// + ueL3      : UmtsLayer3Ue*     : Pointer to the UE umts layer3 data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeReleaseRcvPh(
    Node* node,
    UmtsLayer3Data *umtsL3,
    UmtsLayer3Ue* ueL3)
{
    std::list <UmtsLayer3UeNodebInfo*>::iterator it;

    for (it = ueL3->ueNodeInfo->begin();
            it != ueL3->ueNodeInfo->end(); ++it)
    {
        if ((*it)->cellStatus == UMTS_CELL_ACTIVE_SET)
        {
            UmtsMacPhyReleaseRcvPhCh(
                node,
                umtsL3->interfaceIndex,
                (*it)->cellId);
            (*it)->cellStatus = UMTS_CELL_MONITORED_SET;
        }
    }

    // GUI
    if (node->guiOption)
    {
        GUI_SendIntegerData(node->nodeId,
                            ueL3->stat.activeSetSizeGuiId,
                            UmtsLayer3UeGetCellStatusSetSize(
                               node, umtsL3, ueL3, UMTS_CELL_ACTIVE_SET),
                            node->getNodeTime());
    }
}

// /**
// FUNCTION   :: UmtsLayer3UeReleaseDedctRsrc
// LAYER      :: Layer 3
// PURPOSE    :: Release dedicated resources
// + node      : Node*             : Pointer to node.
// + umtsL3    : UmtsLayer3Data *  : Pointer to the umts layer3 data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeReleaseDedctRsrc(
    Node* node,
    UmtsLayer3Data *umtsL3)
{
    UmtsLayer3Ue *ueL3 = (UmtsLayer3Ue *) (umtsL3->dataPtr);

    UmtsLayer3UeReleaseRcvPh(node, umtsL3, ueL3);

    //Release all dedicated uplink radio links
    UmtsMacPhyUeRelAllDedctPhCh(node, umtsL3->interfaceIndex);

    //Release all dedicated uplink DCHs
    UmtsMacPhyUeRelAllDch(node, umtsL3->interfaceIndex);

    for (int i = 0; i < UMTS_MAX_RAB_SETUP; i++)
    {
        if (ueL3->rrcData->rabInfos[i])
        {
            MEM_free(ueL3->rrcData->rabInfos[i]);
            ueL3->rrcData->rabInfos[i] = NULL;
        }
    }

    //Release all logical channels and RLC entities for data RBs
    for (int i = 0; i < UMTS_MAX_RB_ALL_RAB; i++)
    {
        if (ueL3->rrcData->rbRabId[i] != UMTS_INVALID_RAB_ID)
        {
            UmtsLayer3UeReleaseRsrcByRbId(
                node,
                umtsL3,
                ueL3,
                (char)(i + UMTS_RBINRAB_START_ID));
            ueL3->rrcData->rbRabId[i] = UMTS_INVALID_RAB_ID;
        }
    }

    //Release all logical channels and RLC entitites for SRBs
    for (int i = 0; i < 3; i++)
    {
        UmtsLayer3UeReleaseRsrcByRbId(
            node,
            umtsL3,
            ueL3,
            (char)(i+1));
    }
}

// FUNCTION   :: UmtsLayer3UeSigConnRelInd
// LAYER      :: Layer3 RRC
// PURPOSE    :: Notify MM/GMM one Signal connection is  released
// PARAMETERS ::
// + node      : Node*               : Pointer to node.
// + umtsL3    : UmtsLayer3Data *    : Pointer to UMTS Layer3 data
// + ueL3      : UmtsLayer3Ue*       : Pointer to the UE UMTS layer3 Data
// + domain    : UmtsLayer3CnDomainId : Domain id
// RETURN     :: NULL
// **/
static
void UmtsLayer3UeSigConnRelInd(
            Node* node,
            UmtsLayer3Data *umtsL3,
            UmtsLayer3Ue* ueL3,
            UmtsLayer3CnDomainId domain)
{
    if (domain == UMTS_LAYER3_CN_DOMAIN_PS)
    {
        ueL3->gmmData->pmmState = UMTS_PMM_IDLE;
        if (ueL3->gmmData->T3310)
        {
            MESSAGE_CancelSelfMsg(node, ueL3->gmmData->T3310);
            ueL3->gmmData->T3310 = NULL;
        }
        if (ueL3->gmmData->T3317)
        {
            MESSAGE_CancelSelfMsg(node, ueL3->gmmData->T3317);
            ueL3->gmmData->T3317 = NULL;
        }
        if (ueL3->gmmData->T3319)
        {
            MESSAGE_CancelSelfMsg(node, ueL3->gmmData->T3319);
            ueL3->gmmData->T3319 = NULL;
        }

        std::list<UmtsLayer3UeAppInfo*>::iterator it;

        if (ueL3->gmmData->AttCompWaitRelInd)
        {
            ueL3->gmmData->AttCompWaitRelInd = FALSE;
        }

        for (it = ueL3->appInfoList->begin();
             it != ueL3->appInfoList->end();
             ++it)
        {
            if ((*it)->classifierInfo->domainInfo == domain)
            {
                UmtsLayer3UeAppInfoFinalize(node, ueL3, (*it));
                *it = NULL;
            }
        }
        ueL3->appInfoList->remove(NULL);
    }

    //release CS domain application
    else
    {
        UmtsMmMsData* ueMm = ueL3->mmData;
        while (!ueL3->mmData->actMmConns->empty())
        {
            UmtsLayer3MmConn* mmConn = ueL3->mmData->actMmConns->front();
            UmtsLayer3UeMmConnRelInd(node, umtsL3, mmConn->pd, mmConn->ti);
            delete mmConn;
            ueL3->mmData->actMmConns->pop_front();
        }
        if (ueMm->T3230)
        {
            MESSAGE_CancelSelfMsg(node, ueMm->T3230);
            ueMm->T3230 = NULL;
        }
        if (ueMm->T3240)
        {
            MESSAGE_CancelSelfMsg(node, ueMm->T3240);
            ueMm->T3240 = NULL;
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3UePrintParameter
// LAYER      :: Layer 3
// PURPOSE    :: Print out UE specific parameters
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + ueL3      : UmtsLayer3Ue *   : Pointer to UMTS UE Layer3 data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UePrintParameter(Node *node, UmtsLayer3Ue *ueL3)
{
    printf("Node%d(UE)'s specific parameters:\n", node->nodeId);
    printf("    Qrxlevmin = %f\n", ueL3->para.Qrxlevmin);
    printf("    Ssearch = %f\n", ueL3->para.Ssearch);
    printf("    Qhyst = %f\n", ueL3->para.Qhyst);

    return;
}

// /**
// FUNCTION   :: UmtsLayer3UeInitParameter
// LAYER      :: Layer 3
// PURPOSE    :: Initialize UE specific parameters
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + nodeInput : const NodeInput* : Pointer to node input.
// + ueL3      : UmtsLayer3Ue *   : Pointer to UMTS UE Layer3 data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeInitParameter(Node *node,
                               const NodeInput *nodeInput,
                               UmtsLayer3Ue *ueL3)
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
        if (ueL3->duplexMode == UMTS_DUPLEX_MODE_FDD &&
            (doubleVal < UMTS_MIN_FDD_CELL_SELECTION_MIN_RX_LEVEL ||
             doubleVal > UMTS_MAX_FDD_CELL_SELECTION_MIN_RX_LEVEL))
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString,
                    "UMTS Node%d(UE): The value of parameter "
                    "UMTS-CELL-SELECTION-MIN-RX-LEVEL must be "
                    "between [%.2f, %.2f]. Default value %.2f is used.\n",
                    node->nodeId,
                    UMTS_MIN_FDD_CELL_SELECTION_MIN_RX_LEVEL,
                    UMTS_MAX_FDD_CELL_SELECTION_MIN_RX_LEVEL,
                    UMTS_DEFAULT_FDD_CELL_SELECTION_MIN_RX_LEVEL);
            ERROR_ReportWarning(errString);

            ueL3->para.Qrxlevmin =
                UMTS_DEFAULT_FDD_CELL_SELECTION_MIN_RX_LEVEL;
        }
        else
        {
            // stored the value
            ueL3->para.Qrxlevmin = doubleVal;
        }
    }
    else
    {
        // use the default value
        if (ueL3->duplexMode == UMTS_DUPLEX_MODE_FDD)
        {
            ueL3->para.Qrxlevmin =
                UMTS_DEFAULT_FDD_CELL_SELECTION_MIN_RX_LEVEL;
        }
        else
        {
            ueL3->para.Qrxlevmin =
                UMTS_DEFAULT_TDD_CELL_SELECTION_MIN_RX_LEVEL;
        }
    }

#if 0
    // read the minimum Qqualmin level for cell selection (Qrxlevmin)
    // not used in this release
    IO_ReadDouble(node->nodeId,
                  ANY_ADDRESS,
                  nodeInput,
                  "UMTS-CELL-SELECTION-MIN-QUAL-LEVEL",
                  &wasFound,
                  &doubleVal);

    if (wasFound)
    {
        // do validation check first
        // stored the value
        ueL3->para.Qqualmin = doubleVal;
    }
    else
    {
        // use the default value
        if (ueL3->duplexMode == UMTS_DUPLEX_MODE_FDD)
        {
            ueL3->para.Qqualmin =
                UMTS_DEFAULT_FDD_CELL_SELECTION_MIN_QUAL_LEVEL;
        }
        else
        {
            // no use for TDD
            ueL3->para.Qqualmin =
                UMTS_DEFAULT_FDD_CELL_SELECTION_MIN_QUAL_LEVEL;
        }
    }
#endif

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
        ueL3->para.Ssearch = doubleVal;
    }
    else
    {
        if (wasFound)
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString,
                    "UMTS Node%d(UE): The value of parameter "
                    "UMTS-CELL-SEARCH-THRESHOLD must be "
                    "between [%.2f, %.2f]. Default value %.2f is used.\n",
                    node->nodeId,
                    UMTS_MIN_CELL_SEARCH_THRESHOLD,
                    UMTS_MAX_CELL_SEARCH_THRESHOLD,
                    UMTS_DEFAULT_CELL_SEARCH_THRESHOLD);
            ERROR_ReportWarning(errString);
        }

        // use the default value, only support FDD currently
        ueL3->para.Ssearch = UMTS_DEFAULT_CELL_SEARCH_THRESHOLD;
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
        ueL3->para.Qhyst = doubleVal;
    }
    else
    {
        if (wasFound)
        {
            char errString[MAX_STRING_LENGTH];
            sprintf(errString,
                    "UMTS Node%d(UE): Value of parameter "
                    "UMTS-CELL-RESELECTION-HYSTERESIS is invalid. "
                    "It must be a number larger than %.2f and "
                    "less than %.2f. Default value as %.2f is used.\n",
                    node->nodeId,
                    UMTS_MIN_CELL_RESELECTION_HYSTERESIS,
                    UMTS_MAX_CELL_RESELECTION_HYSTERESIS,
                    UMTS_DEFAULT_CELL_RESELECTION_HYSTERESIS);
            ERROR_ReportWarning(errString);
        }

        // use the default value
        ueL3->para.Qhyst = UMTS_DEFAULT_CELL_RESELECTION_HYSTERESIS;
    }

    ueL3->para.cellEvalTime = UMTS_DEFAULT_CELL_EVALUATION_TIME;
    ueL3->para.shoEvalTime = UMTS_DEFAULT_CELL_EVALUATION_TIME;

    return;
}

// /**
// FUNCTION   :: UmtsLayer3UeInitRrc
// LAYER      :: Layer 3
// PURPOSE    :: Initialize UE RRC data
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + nodeInput : const NodeInput* : Pointer to node input.
// + ueL3      : UmtsLayer3Ue *   : Pointer to UMTS UE Layer3 data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeInitRrc(Node *node,
                         const NodeInput *nodeInput,
                         UmtsLayer3Ue *ueL3)
{
    UmtsRrcUeData* rrcData = (UmtsRrcUeData*)
                                (MEM_malloc(sizeof(UmtsRrcUeData)));
    memset(rrcData, 0, sizeof(UmtsRrcUeData));
    rrcData->state = UMTS_RRC_STATE_IDLE;
    rrcData->subState = UMTS_RRC_SUBSTATE_NO_PLMN;

    ueL3->rrcData = rrcData;
    rrcData->ueId = node->nodeId;
    rrcData->ueScCode = rrcData->ueId;

    rrcData->acceptedTranst =
        new std::map<UmtsRRMessageType, unsigned char>;
    rrcData->rejectedTranst =
        new std::map<UmtsRRMessageType, unsigned char>;

    for (int i = 0; i < UMTS_MAX_RB_ALL_RAB; i++)
    {
        rrcData->rbRabId[i] = UMTS_INVALID_RAB_ID;
    }
}

// /**
// FUNCTION   :: UmtsLayer3UeFinalizeRrc
// LAYER      :: Layer 3
// PURPOSE    :: Finnalize UE RRC data
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// +umtsL3    : UmtsLayer3Data *    : Pointer to UMTS Layer3 data
// + ueL3      : UmtsLayer3Ue *   : Pointer to UMTS UE Layer3 data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeFinalizeRrc(Node *node,
                             UmtsLayer3Data* umtsL3,
                             UmtsLayer3Ue *ueL3)
{
    UmtsRrcUeData* rrcData = ueL3->rrcData;

    rrcData->acceptedTranst->clear();
    delete rrcData->acceptedTranst;
    rrcData->rejectedTranst->clear();
    delete rrcData->rejectedTranst;

    for (int i = 0; i<UMTS_MAX_RAB_SETUP; i++)
    {
        if (rrcData->rabInfos[i])
        {
            MEM_free(rrcData->rabInfos[i]);
            rrcData->rabInfos[i] = NULL;
        }
    }

    if (rrcData->timerT300)
    {
        MESSAGE_CancelSelfMsg(node,rrcData->timerT300);
        rrcData->timerT300 = NULL;
    }
    if (rrcData->timerT308)
    {
        MESSAGE_CancelSelfMsg(node,rrcData->timerT308);
        rrcData->timerT308 = NULL;
    }
    if (rrcData->timerRrcConnReqWaitTime)
    {
        MESSAGE_CancelSelfMsg(node,rrcData->timerRrcConnReqWaitTime);
        rrcData->timerRrcConnReqWaitTime = NULL;
    }

    MEM_free(rrcData);
    ueL3->rrcData = NULL;
}

// /**
// FUNCTION   :: UmtsLayer3UeEnterIdle
// LAYER      :: Layer 3
// PURPOSE    :: UE RRC enters into IDLE state
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// +umtsL3    : UmtsLayer3Data *    : Pointer to UMTS Layer3 data
// + ueL3      : UmtsLayer3Ue *   : Pointer to UMTS UE Layer3 data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeEnterIdle(Node *node,
                           UmtsLayer3Data* umtsL3,
                           UmtsLayer3Ue *ueL3)
{
    UmtsRrcUeData* rrcData = ueL3->rrcData;

    rrcData->state = UMTS_RRC_STATE_IDLE;
    rrcData->subState = UMTS_RRC_SUBSTATE_NO_PLMN;

    rrcData->amRlcErrRb234 = FALSE;
    rrcData->amRlcErrRb5up = FALSE;
    rrcData->orderedReconfiguration = FALSE;
    rrcData->cellUpStarted = FALSE;
    rrcData->protocolErrorIndi = FALSE;
    rrcData->protocolErrorRej = FALSE;
    rrcData->failureIndi = FALSE;
    rrcData->estCause = UMTS_RRC_ESTCAUSE_CLEARED;

    if (rrcData->timerT308)
    {
        MESSAGE_CancelSelfMsg(node,rrcData->timerT308);
        rrcData->timerT308 = NULL;
    }
    rrcData->counterV308 = 0;

    if (rrcData->rachOn)
    {
        UmtsLayer3UeReleaseRandomAccessChannel(
            node,
            umtsL3,
            ueL3);
        rrcData->rachOn = FALSE;
    }

    //Clear all transactions
    rrcData->acceptedTranst->clear();
    rrcData->rejectedTranst->clear();

    //Remove all signalling connections when leaving RRC connected state
    for (int i = 0; i < UMTS_MAX_CN_DOMAIN; i++)
    {
        rrcData->signalConn[i]  = FALSE;
    }
    //ueL3->stat.numCellSearch++;
}

// /**
// FUNCTION::       UmtsLayer3UeSendPacketToLayer2
// LAYER::          UMTS Layer3 RRC
// PURPOSE::        Send packet to layer2
// PARAMETERS::
// + node:       pointer to the network node
// + interfaceIndex: index of interface
// + rbId:       radio bearer Id asscociated with this RLC entity
// + msg:        The SDU message from upper layer
// + ueId:       UE identifier, used to look up RLC entity at UTRAN side
// RETURN::         NULL
// **/
static
void  UmtsLayer3UeSendPacketToLayer2(
    Node* node,
    int interfaceIndex,
    unsigned int rbId,
    Message* msg)
{
    UmtsLayer2UpperLayerHasPacketToSend(node,
                                        interfaceIndex,
                                        rbId,
                                        msg,
                                        node->nodeId);
}

// FUNCTION   :: UmtsLayer3UeSendRrcMsg
// LAYER      :: Layer3 RRC
// PURPOSE    :: Requested by upper layer to send NAS message.
//               Note: RRC connection must be established before this
//               function is called
// PARAMETERS ::
// + node      : Node*               : Pointer to node.
// + umtsL3    : UmtsLayer3Data *    : Pointer to UMTS Layer3 data
// + msgType   : char                : NAS message type
// + msg       : Message*            : Pointer to the RRC message
// + srbId     : char                : signalling Rb Id
// RETURN     :: BOOL               : TRUE if the message has been sent
// **/
static
void UmtsLayer3UeSendRrcMsg(
    Node* node,
    UmtsLayer3Data *umtsL3,
    char msgType,
    Message* msg,
    char srbId = UMTS_SRB2_ID)
{
    UmtsLayer3AddHeader(
        node,
        msg,
        UMTS_LAYER3_PD_RR,
        0,
        msgType);
    UmtsLayer3UeSendPacketToLayer2(node,
                                   umtsL3->interfaceIndex,
                                   srbId,
                                   msg);
}

// FUNCTION   :: UmtsLayer3UeAlertAppCall
// LAYER      :: Layer 3
// PURPOSE    :: Alert the UMTSCALL application
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data*        : Pointer to UMTS Layer3 data
// + ueFlow         : UmtsLayer3UeAppInfo*   : Pointer to the call flow data
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3UeAlertAppCall(
        Node* node,
        UmtsLayer3Data* umtsL3,
        UmtsLayer3UeAppInfo* ueFlow)
{
    UmtsCcMsData* ueCc = ueFlow->cmData.ccData;
    Message* msgToApp = MESSAGE_Alloc(
                            node,
                            APP_LAYER,
                            APP_UMTS_CALL,
                            MSG_APP_CELLULAR_FromNetworkCallArrive);
    MESSAGE_InfoAlloc(node,
                      msgToApp,
                      sizeof(AppUmtsCallArriveMessageInfo));
    AppUmtsCallArriveMessageInfo* callArriveInfo =
        (AppUmtsCallArriveMessageInfo*)MESSAGE_ReturnInfo(msgToApp);
    callArriveInfo->appId = ueCc->callAppId;
    callArriveInfo->transactionId = ueFlow->transId;
    callArriveInfo->appSrcNodeId = ueCc->peerAddr;
    callArriveInfo->appDestNodeId = node->nodeId;
    callArriveInfo->callEndTime = ueCc->endTime;
    callArriveInfo->avgTalkTime = ueCc->avgTalkTime;

    clocktype delay = 1 * MILLI_SECOND;
    MESSAGE_Send(node, msgToApp, delay);
}

// FUNCTION   :: UmtsLayer3UeNotifyAppCallAnswerred
// LAYER      :: Layer 3
// PURPOSE    :: Notify APP that the call is anwserred by peer
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data*        : Pointer to UMTS Layer3 data
// + ueFlow         : UmtsLayer3UeAppInfo*   : Pointer to the call flow data
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3UeNotifyAppCallAnswerred(
        Node* node,
        UmtsLayer3Data* umtsL3,
        UmtsLayer3UeAppInfo* ueFlow)
{
    UmtsCcMsData* ueCc = ueFlow->cmData.ccData;
    Message* msgToApp = MESSAGE_Alloc(
                            node,
                            APP_LAYER,
                            APP_UMTS_CALL,
                            MSG_APP_CELLULAR_FromNetworkCallAccepted);
    MESSAGE_InfoAlloc(node,
                      msgToApp,
                      sizeof(AppUmtsCallAcceptMessageInfo));
    AppUmtsCallAcceptMessageInfo* callInfo =
                (AppUmtsCallAcceptMessageInfo*)
                MESSAGE_ReturnInfo(msgToApp);
    callInfo->appId = ueCc->callAppId;
    callInfo->appSrcNodeId = node->nodeId;
    callInfo->appDestNodeId = ueCc->peerAddr;
    callInfo->transactionId = ueFlow->transId;
    MESSAGE_Send(node, msgToApp, 1*MILLI_SECOND);
}

// FUNCTION   :: UmtsLayer3UeNotifyAppCallEnd
// LAYER      :: Layer 3
// PURPOSE    :: Report a CALL ending to the UMTSCALL application
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data*        : Pointer to UMTS Layer3 data
// + ueFlow         : UmtsLayer3UeAppInfo*   : Pointer to the call flow data
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3UeNotifyAppCallEnd(
        Node* node,
        UmtsLayer3Data* umtsL3,
        UmtsLayer3UeAppInfo* ueFlow)
{
    UmtsCcMsData* ueCc = ueFlow->cmData.ccData;
    Message* msgToApp = MESSAGE_Alloc(
                            node,
                            APP_LAYER,
                            APP_UMTS_CALL,
                            MSG_APP_CELLULAR_FromNetworkCallEndByRemote);
    MESSAGE_InfoAlloc(
        node,
        msgToApp,
        sizeof(AppUmtsCallEndByRemoteMessageInfo));
    AppUmtsCallEndByRemoteMessageInfo* callInfo =
                (AppUmtsCallEndByRemoteMessageInfo*)
                MESSAGE_ReturnInfo(msgToApp);
    callInfo->appId = ueCc->callAppId;
    callInfo->transactionId = ueFlow->transId;
    if (ueFlow->srcDestType == UMTS_FLOW_MOBILE_ORIGINATED)
    {
        callInfo->appSrcNodeId = node->nodeId;
        callInfo->appDestNodeId = ueCc->peerAddr;
    }
    else
    {
        callInfo->appSrcNodeId = ueCc->peerAddr;
        callInfo->appDestNodeId = node->nodeId;
    }
    MESSAGE_Send(node, msgToApp, 1*MILLI_SECOND);
}

// FUNCTION   :: UmtsLayer3UeNotifyAppCallDrop
// LAYER      :: Layer 3
// PURPOSE    :: Report a CALL drop to the UMTSCALL application
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data*        : Pointer to UMTS Layer3 data
// + ueFlow         : UmtsLayer3UeAppInfo*   : Pointer to the CALL flow data
// + cause          : AppUmtsCallDropCauseType : drop cause
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3UeNotifyAppCallDrop(
        Node* node,
        UmtsLayer3Data* umtsL3,
        UmtsLayer3UeAppInfo* ueFlow,
        AppUmtsCallDropCauseType cause)
{
    UmtsCcMsData* ueCc = ueFlow->cmData.ccData;
    Message* msgToApp = MESSAGE_Alloc(
                            node,
                            APP_LAYER,
                            APP_UMTS_CALL,
                            MSG_APP_CELLULAR_FromNetworkCallDropped);
    MESSAGE_InfoAlloc(node,
                      msgToApp,
                      sizeof(AppUmtsCallDropMessageInfo));
    AppUmtsCallDropMessageInfo* callInfo =
        (AppUmtsCallDropMessageInfo*)
            MESSAGE_ReturnInfo(msgToApp);
    callInfo->appId = ueCc->callAppId;
    callInfo->transactionId = ueFlow->transId;
    callInfo->dropCause = cause;
    if (ueFlow->srcDestType == UMTS_FLOW_MOBILE_ORIGINATED)
    {
        callInfo->appSrcNodeId = node->nodeId;
        callInfo->appDestNodeId = ueCc->peerAddr;
    }
    else
    {
        callInfo->appSrcNodeId = ueCc->peerAddr;
        callInfo->appDestNodeId = node->nodeId;
    }
    MESSAGE_Send(node, msgToApp, 1*MILLI_SECOND);
}

// FUNCTION   :: UmtsLayer3UeNotifyAppCallReject
// LAYER      :: Layer 3
// PURPOSE    :: Report a CALL rejection to the UMTSCALL application
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data*        : Pointer to UMTS Layer3 data
// + ueFlow         : UmtsLayer3UeAppInfo*   : Pointer to the CALL flow data
// + cause          : AppUmtsCallRejectCauseType : reject cause
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3UeNotifyAppCallReject(
        Node* node,
        UmtsLayer3Data* umtsL3,
        UmtsLayer3UeAppInfo* ueFlow,
        AppUmtsCallRejectCauseType cause)
{
    UmtsCcMsData* ueCc = ueFlow->cmData.ccData;
    Message* msgToApp = MESSAGE_Alloc(
                            node,
                            APP_LAYER,
                            APP_UMTS_CALL,
                            MSG_APP_CELLULAR_FromNetworkCallRejected);
    MESSAGE_InfoAlloc(node,
                      msgToApp,
                      sizeof(AppUmtsCallRejectMessageInfo));
    AppUmtsCallRejectMessageInfo* callInfo =
        (AppUmtsCallRejectMessageInfo*)
            MESSAGE_ReturnInfo(msgToApp);
    callInfo->appId = ueCc->callAppId;
    callInfo->transactionId = ueFlow->transId;
    callInfo->rejectCause = cause;
    if (ueFlow->srcDestType == UMTS_FLOW_MOBILE_ORIGINATED)
    {
        callInfo->appSrcNodeId = node->nodeId;
        callInfo->appDestNodeId = ueCc->peerAddr;
    }
    else
    {
        callInfo->appSrcNodeId = ueCc->peerAddr;
        callInfo->appDestNodeId = node->nodeId;
    }
    MESSAGE_Send(node, msgToApp, 1*MILLI_SECOND);
}

// FUNCTION   :: UmtsLayer3UeNotifyAppCallReject
// LAYER      :: Layer 3
// PURPOSE    :: Report a CALL rejection to the UMTSCALL application
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data*        : Pointer to UMTS Layer3 data
// + ueFlow         : UmtsLayer3UeAppInfo*   : Pointer to the CALL flow data
// + cause          : AppUmtsCallRejectCauseType : reject cause
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3UeNotifyAppCallReject(
        Node* node,
        UmtsLayer3Data* umtsL3,
        AppUmtsCallStartMessageInfo* callInfo,
        AppUmtsCallRejectCauseType cause)
{
    Message* msgToApp = MESSAGE_Alloc(
                            node,
                            APP_LAYER,
                            APP_UMTS_CALL,
                            MSG_APP_CELLULAR_FromNetworkCallRejected);
    MESSAGE_InfoAlloc(node,
                      msgToApp,
                      sizeof(AppUmtsCallRejectMessageInfo));
    AppUmtsCallRejectMessageInfo* rejectInfo =
        (AppUmtsCallRejectMessageInfo*)MESSAGE_ReturnInfo(msgToApp);
    rejectInfo->appId = callInfo->appId;
    rejectInfo->appSrcNodeId = callInfo->appSrcNodeId;
    rejectInfo->appDestNodeId = callInfo->appDestNodeId;
    rejectInfo->rejectCause = cause;
    MESSAGE_Send(node, msgToApp, 1 * MILLI_SECOND);
}

// FUNCTION   :: UmtsLayer3UeNotifyAppCallClearing
// LAYER      :: Layer 3
// PURPOSE    :: Report a CALL clearing event to the UMTSCALL application
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data*        : Pointer to UMTS Layer3 data
// + ueFlow         : UmtsLayer3UeAppInfo*   :
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3UeNotifyAppCallClearing(
        Node* node,
        UmtsLayer3Data* umtsL3,
        UmtsLayer3UeAppInfo* ueFlow,
        UmtsCallClearingType clearType)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*)(umtsL3->dataPtr);
    if (clearType == UMTS_CALL_CLEAR_END)
    {
        if (ueFlow->srcDestType == UMTS_FLOW_MOBILE_ORIGINATED)
        {
            ueL3->stat.numMoAppSucEnded ++;
            ueL3->stat.numMoCsAppSucEnded ++;
        }
        else
        {
            ueL3->stat.numMtAppSucEnded ++;
            ueL3->stat.numMtCsAppSucEnded ++;
        }
        UmtsLayer3UeNotifyAppCallEnd(node, umtsL3, ueFlow);
    }
    else if (clearType == UMTS_CALL_CLEAR_DROP)
    {
        if (ueFlow->srcDestType == UMTS_FLOW_MOBILE_ORIGINATED)
        {
            ueL3->stat.numMoAppDropped ++;
            ueL3->stat.numMoCsAppDropped ++;
        }
        else
        {
            ueL3->stat.numMtAppDropped ++;
            ueL3->stat.numMtCsAppDropped ++;
        }
        UmtsLayer3UeNotifyAppCallDrop(
            node,
            umtsL3,
            ueFlow,
            APP_UMTS_CALL_DROP_CAUSE_UNKNOWN);
    }
    else if (clearType == UMTS_CALL_CLEAR_REJECT)
    {
        if (ueFlow->srcDestType == UMTS_FLOW_MOBILE_ORIGINATED)
        {
            ueL3->stat.numMoAppRejected ++;
            ueL3->stat.numMoCsAppRejected ++;
        }
        else
        {
            ueL3->stat.numMtAppRejected ++;
            ueL3->stat.numMtCsAppRejected ++;
        }
        UmtsLayer3UeNotifyAppCallReject(
            node,
            umtsL3,
            ueFlow,
            APP_UMTS_CALL_REJECT_CAUSE_UNKNOWN);
    }
    else
    {
        ERROR_ReportError("Receives a unknown call clearing type.");
    }
}

// FUNCTION   :: UmtsLayer3UeForwardCsPktToApp
// LAYER      :: Layer 3
// PURPOSE    :: Forward a CS packet to the application layer
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data*        : Pointer to UMTS Layer3 data
// + rabId          : char                   : RAB ID
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3UeForwardCsPktToApp(
        Node* node,
        UmtsLayer3Data* umtsL3,
        char rabId,
        Message* msg)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*)(umtsL3->dataPtr);
    UmtsLayer3UeAppInfo* ueFlow = UmtsLayer3UeFindAppByRabId(
                                        node,
                                        ueL3,
                                        rabId);
    if (ueFlow)
    {
        // Currently CS domain data are exclusively for UMTS_CALL_APP
        UmtsCcMsData* ueCc = ueFlow->cmData.ccData;
        MESSAGE_SetLayer(msg, APP_LAYER, APP_UMTS_CALL);
        MESSAGE_SetEvent(msg, MSG_APP_CELLULAR_FromNetworkPktArrive);

        MESSAGE_InfoAlloc(node, msg, sizeof(AppUmtsCallPktArrivalInfo));
        AppUmtsCallPktArrivalInfo* callInfo =
            (AppUmtsCallPktArrivalInfo*)MESSAGE_ReturnInfo(msg);
        callInfo->appId = ueCc->callAppId;
        if (ueFlow->srcDestType == UMTS_FLOW_MOBILE_ORIGINATED)
        {
            callInfo->appSrcNodeId = node->nodeId;
        }
        else
        {
            callInfo->appSrcNodeId = ueCc->peerAddr;
        }
        MESSAGE_Send(node, msg, 1*MILLI_SECOND);
    }
    else
    {
        MESSAGE_Free(node, msg);
    }
}

// /**
// FUNCTION   :: UmtsLayer3UeSearchNewCell
// LAYER      :: Layer3
// PURPOSE    :: Search new available cell if RRC
//               connection is lost or old selected sell is not available.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data*   : Pointer to L3 data
// + ueL3             : UmtsLayer3Ue *    : Pointer to l3 Ue data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeSearchNewCell(
          Node* node,
          UmtsLayer3Data* umtsL3,
          UmtsLayer3Ue *ueL3)
{
    // upate detect list, active lsit and monitor list
    double strongestRscp = ueL3->para.Qrxlevmin;
    std::list <UmtsLayer3UeNodebInfo*>::iterator itStrongest;
    std::list <UmtsLayer3UeNodebInfo*>::iterator it;
    clocktype curTime = node->getNodeTime();

    itStrongest = ueL3->ueNodeInfo->end();
    for (it = ueL3->ueNodeInfo->begin();
         it != ueL3->ueNodeInfo->end();
         it++)
    {
        // update the measurement
        (*it)->cpichRscp = (*it)->cpichRscpMeas->GetAvgMeas(curTime);
        if (strongestRscp < (*it)->cpichRscp)
        {
            strongestRscp = (*it)->cpichRscp;
            itStrongest = it;
        }
    }

    if (ueL3->ueNodeInfo->empty() || itStrongest == ueL3->ueNodeInfo->end())
    {
        ueL3->priNodeB = NULL;
        ueL3->rrcData->subState = UMTS_RRC_SUBSTATE_NO_PLMN;
        if (ueL3->ueNodeInfo->empty())
        {
            ueL3->mmData->idleSubState = UMTS_MM_MS_IDLE_NO_CELL_AVAILABLE;
        }
        else
        {
            ueL3->mmData->idleSubState = UMTS_MM_MS_IDLE_PLMN_SEARCH;
        }
        ueL3->mmData->updateState = UMTS_MM_NOT_UPDATED;

        // upate stat
        ueL3->stat.numCellSearch ++;

        if (DEBUG_CELL_SELECT)
        {
            UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
            printf("\n\t In SearchNewCell(), No Cell can be selected\n");
        }
    }
    else
    {
        if (DEBUG_CELL_SELECT)
        {
            UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
            printf("\n\t In SearchNewCell(), reselect a cell/NodeB with "
                "CellID/priSc %d regAreaId %d,"
                "old one is %d %d\n",
                (*itStrongest)->cellId, (*itStrongest)->regAreaId,
                ueL3->rrcData->selectedCellId, ueL3->locRoutAreaId);
        }

        ueL3->priNodeB = (*itStrongest);
        ueL3->rrcData->subState =
            UMTS_RRC_SUBSTATE_SUITABLE_CELL_SELECTED;
        // ready to update sibs
        (*itStrongest)->sibInd = 0xFFE8;
        ueL3->rrcData->selectedCellId = (*itStrongest)->cellId;
        ueL3->rrcData->counterV300 = 0;
        ueL3->rrcData->counterV308 = 0;

        // upate stat
        ueL3->stat.numCellSearch ++;

        ueL3->stat.numCellReselection ++;

        if ((*itStrongest)->regAreaId ==
            UMTS_INVALID_REG_AREA_ID ||
            (ueL3->locRoutAreaId != (*itStrongest)->regAreaId ))
        {
            if (DEBUG_CELL_SELECT && (*itStrongest)->regAreaId ==
            UMTS_INVALID_REG_AREA_ID)
            {
                UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
                printf("\n\t In SearchNewCell(),"
                    "New Cell's RAI unknow yet, Possible Loc Up\n");
            }
            else if (DEBUG_CELL_SELECT &&
                ueL3->locRoutAreaId != (*itStrongest)->regAreaId)
            {
                UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
                printf("\n\t In SearchNewCell(), "
                    "Entering New Cell's RAI, Loc Up needed\n");
            }

            ueL3->mmData->idleSubState =
                UMTS_MM_MS_IDLE_LOCATION_UPDATE_NEEDED;
        }
        else
        {
            if (DEBUG_CELL_SELECT)
            {
                UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
                printf("\n\t In SearchNewCell(), "
                    "Still in Old's RAI, No Need Loc Up\n");
            }
            ueL3->mmData->idleSubState =
                UMTS_MM_MS_IDLE_PLMN_SEARCH_NORMAL_SERVICE;
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3UeCheckCellSelectReselect
// LAYER      :: Layer3
// PURPOSE    :: Handle measurement msg.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data*   : Pointer to L3 data
// + ueL3             : UmtsLayer3Ue *    : Pointer to l3 Ue data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeCheckCellSelectReselect(
          Node* node,
          UmtsLayer3Data* umtsL3,
          UmtsLayer3Ue *ueL3)
{

    // upate detect list, active lsit and monitor list
    double strongestRscp = ueL3->para.Qrxlevmin;
    std::list <UmtsLayer3UeNodebInfo*>::iterator itStrongest;
    std::list <UmtsLayer3UeNodebInfo*>::iterator it;
    clocktype curTime = node->getNodeTime();

    itStrongest = ueL3->ueNodeInfo->end();
    for (it = ueL3->ueNodeInfo->begin();
         it != ueL3->ueNodeInfo->end();
         it++)
    {
        // update the measurement
        (*it)->cpichRscp = (*it)->cpichRscpMeas->GetAvgMeas(curTime);
        if (strongestRscp < (*it)->cpichRscp)
        {
            strongestRscp = (*it)->cpichRscp;
            itStrongest = it;
        }
    }


    if (ueL3->ueNodeInfo->empty() || itStrongest == ueL3->ueNodeInfo->end())
    {
        ueL3->priNodeB = NULL;
        ueL3->rrcData->subState = UMTS_RRC_SUBSTATE_NO_PLMN;
        if (ueL3->ueNodeInfo->empty())
        {
            ueL3->mmData->idleSubState = UMTS_MM_MS_IDLE_NO_CELL_AVAILABLE;
        }
        else
        {
            ueL3->mmData->idleSubState = UMTS_MM_MS_IDLE_PLMN_SEARCH;
        }

        ueL3->mmData->updateState = UMTS_MM_NOT_UPDATED;

        // upate stat
        ueL3->stat.numCellSearch ++;

        if (DEBUG_CELL_SELECT)
        {
            UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
            printf("\n\t In CellSelectReselect, No Cell can be selected,"
                "keep searching\n");
        }
    }
    else if (ueL3->priNodeB &&
             itStrongest != ueL3->ueNodeInfo->end() &&
             (*itStrongest) != ueL3->priNodeB)
    {
        // TODO: when IDEL, directly change the priNodeB point
        // When URA_PCH, CELL_PCH CELL_FACH
        if (DEBUG_CELL_SELECT)
        {
            UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
            printf("\n\t in CellSelectReselect, "
                "reselect a new cell/NodeB with "
                "CellID/priSc %d regAreaId %d,"
                "old one is %d %d\n",
                (*itStrongest)->cellId, (*itStrongest)->regAreaId,
                ueL3->rrcData->selectedCellId, ueL3->locRoutAreaId);
        }

        ueL3->priNodeB = (*itStrongest);
        ueL3->rrcData->subState =
            UMTS_RRC_SUBSTATE_SUITABLE_CELL_SELECTED;
        ueL3->rrcData->selectedCellId = (*itStrongest)->cellId;
        (*itStrongest)->sibInd = 0xFFE8;
        ueL3->rrcData->counterV300 = 0;
        ueL3->rrcData->counterV308 = 0;

        // upate stat
        ueL3->stat.numCellSearch ++;

        ueL3->stat.numCellReselection ++;

        if ((*itStrongest)->regAreaId ==
            UMTS_INVALID_REG_AREA_ID ||
            (ueL3->locRoutAreaId != (*itStrongest)->regAreaId ))
        {
            if (DEBUG_CELL_SELECT && (*itStrongest)->regAreaId ==
                UMTS_INVALID_REG_AREA_ID)
            {
                UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
                printf("\n\t In CellSelectReselect(),"
                    "New Cell's RAI unknow yet, Possible Loc Up\n");
            }
            else if (DEBUG_CELL_SELECT &&
                ueL3->locRoutAreaId != (*itStrongest)->regAreaId)
            {
                UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
                printf("\n\t In CellSelectReselect(), "
                    "Entering New Cell's RAI, Loc Up needed\n");
            }

            ueL3->mmData->idleSubState =
                UMTS_MM_MS_IDLE_LOCATION_UPDATE_NEEDED;
        }
        else
        {
            if (DEBUG_CELL_SELECT)
            {
                UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
                printf("\n\t In CellSelectReselect(), "
                    "Still in Old's RAI, No Need Loc Up\n");
            }
            ueL3->mmData->idleSubState =
                UMTS_MM_MS_IDLE_PLMN_SEARCH_NORMAL_SERVICE;
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3UePrintStats
// LAYER      :: Layer 3
// PURPOSE    :: Print out UE specific statistics
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// +umtsL3     : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + ueL3      : UmtsLayer3Ue *   : Pointer to UMTS UE Layer3 data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UePrintStats(Node *node,
                            UmtsLayer3Data *umtsL3,
                            UmtsLayer3Ue *ueL3)
{
    char buf[MAX_STRING_LENGTH];

    // cell search
    sprintf(
        buf,
        "Number of cell search performed = %d",
        ueL3->stat.numCellSearch);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    sprintf(
        buf,
        "Number of initial cell selections = %d",
        ueL3->stat.numCellInitSelection);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

     sprintf(
        buf,
        "Number of cell reselections = %d",
        ueL3->stat.numCellReselection);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    sprintf(
        buf,
        "Number of PAGING messages received = %d",
        ueL3->stat.numPagingRcvd);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    if (umtsL3->printDetailedStat)
    {
        // system info
        // master system information
        sprintf(
            buf,
            "Number of Master System Information messages received = %d",
            ueL3->stat.numMibRcvd);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of System Information Type 1 messages received = %d",
            ueL3->stat.numSib1Rcvd);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of System Information Type 2 messages received = %d",
            ueL3->stat.numSib2Rcvd);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of System Information Type 3 messages received = %d",
            ueL3->stat.numSib3Rcvd);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

#if 0
        sprintf(
            buf,
            "Number of System Information Type 4 Received = %d",
            ueL3->stat.numSib4Rcvd);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);
#endif

        sprintf(
            buf,
            "Number of System Information Type 5 messages received = %d",
            ueL3->stat.numSib5Rcvd);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        // RRC
        sprintf(
            buf,
            "Number of RRC CONN REQUEST messages sent = %d",
            ueL3->stat.numRrcConnReqSent);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of RRC CONN SETUP messages received = %d",
            ueL3->stat.numRrcConnSetupRcvd);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of RRC CONN SETUP COMPLETE messages sent = %d",
            ueL3->stat.numRrcConnSetupCompSent);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of RRC CONN REJECT messages received = %d",
            ueL3->stat.numRrcConnSetupRejRcvd);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of RRC CONN RELEASE messages received = %d",
            ueL3->stat.numRrcConnRelRcvd);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of RRC CONN RELEASE COMPLETE messages sent = %d",
            ueL3->stat.numRrcConnRelCompSent);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of DL Direct Transfer messages received = %d",
            ueL3->stat.numDlDirectTransferRcvd);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of RADIO BEARER SETUP messages received = %d",
            ueL3->stat.numRadioBearerSetupRcvd);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of RADIO BEARER RELEASE messages received = %d",
            ueL3->stat.numRadioBearerRelRcvd);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of SIGNALLING CONN RELEASE messages received = %d",
            ueL3->stat.numSigConnRelRcvd);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
         "Number of SIGNALLING CONN RELEASE INDICATION messages sent = %d",
            ueL3->stat.numSigConnRelIndSent);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);
    }

    // MM
    sprintf(
        buf,
        "Number of ROUTING AREA UPDATE REQUEST messages sent = %d",
        ueL3->stat.numLocUpReqSent);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    sprintf(
        buf,
        "Number of ROUTING AREA UPDATE ACCEPT messages received = %d",
        ueL3->stat.numLocUpAcptRcvd);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    sprintf(
        buf,
        "Number of ROUTING AREA UPDATE REJECT messages received = %d",
        ueL3->stat.numLocUpRejRcvd);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    // GMM
    sprintf(
        buf,
        "Number of ATTACH REQUEST messages sent = %d",
        ueL3->stat.numAttachReqSent);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    sprintf(
        buf,
        "Number of ATTACH ACCEPT messages received = %d",
        ueL3->stat.numAttachAcptRcvd);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    sprintf(
        buf,
        "Number of ATTACH REJECT messages received = %d",
        ueL3->stat.numAttachRejRcvd);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    sprintf(
        buf,
        "Number of ATTACH COMPLETE messages sent = %d",
        ueL3->stat.numAttachComplSent);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    sprintf(
        buf,
        "Number of SERVICE REQUEST messages sent = %d",
        ueL3->stat.numServiceReqSent);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    sprintf(
        buf,
        "Number of SERVICE ACCEPT messages received = %d",
        ueL3->stat.numServiceAcptRcvd);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    if (umtsL3->printDetailedStat)
    {
        // SM
        sprintf(
            buf,
            "Number of ACTIVATE PDP CONTEXT REQUEST messages sent = %d",
            ueL3->stat.numActivatePDPContextReqSent);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of ACTIVATE PDP CONTEXT ACCEPT messages received = %d",
            ueL3->stat.numActivatePDPContextAcptRcvd);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of ACTIVATE PDP CONTEXT REJECT messages received = %d",
            ueL3->stat.numActivatePDPContextRejRcvd);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
         "Number of REQUEST PDP CONTEXT ACTIVATION messages received = %d",
            ueL3->stat.numRequestPDPConextActivationRcvd);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of DEACTIVATE PDP CONTEXT REQUEST messages sent = %d",
            ueL3->stat.numDeactivatePDPContextReqSent);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
           "Number of DEACTIVATE PDP CONTEXT ACCEPT messages received = %d",
            ueL3->stat.numDeactivatePDPContextAcptRcvd);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
         "Number of DEACTIVATE PDP CONTEXT REQUEST messages received = %d",
            ueL3->stat.numDeactivatePDPContextRequestRcvd);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of DEACTIVATE PDP CONTEXT ACCEPT messages sent = %d",
            ueL3->stat.numDeactivatePDPContextAcptSent);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

         // CC
        sprintf(
            buf,
            "Number of call control SETUP messages sent = %d",
            ueL3->stat.numCcSetupSent);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of call control CALL PROCEEDING messages received = %d",
            ueL3->stat.numCcCallProcRcvd);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of call control ALERTING messages received = %d",
            ueL3->stat.numCcAlertingRcvd);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of call control CONNECT messages received = %d",
            ueL3->stat.numCcConnectRcvd);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of call control CONNECT ACKNOWLEDGE messages sent = %d",
            ueL3->stat.numCcConnAckSent);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of call control DISCONNECT messages sent = %d",
            ueL3->stat.numCcDisconnectSent);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of call control RELEASE messages received = %d",
            ueL3->stat.numCcReleaseRcvd);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of call control RELEASE COMPLETE messages Sent = %d",
            ueL3->stat.numCcReleaseCompleteSent);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of call control SETUP messages received = %d",
            ueL3->stat.numCcSetupRcvd);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of call control CALL CONFIRM messages sent = %d",
            ueL3->stat.numCcCallConfirmSent);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of call control ALERTING messages sent = %d",
            ueL3->stat.numCcAlertingSent);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of call control CONNECT messages sent = %d",
            ueL3->stat.numCcConnectSent);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
        "Number of call control CONNECT ACKNOWLEDGE messages received = %d",
            ueL3->stat.numCcConnAckRcvd);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of call control DISCONNECT messages received = %d",
            ueL3->stat.numCcDisconnectRcvd);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of call control RELEASE messages sent = %d",
            ueL3->stat.numCcReleaseSent);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
         "Number of call control RELEASE COMPLETE messages Received = %d",
            ueL3->stat.numCcReleaseCompleteRcvd);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);
    }

    // MEASUREMENT
    sprintf(
        buf,
        "Number of MEASUREMENT COMMAND messages received = %d",
        ueL3->stat.numMeasComandRcvd);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    sprintf(
        buf,
        "Number of MEASUREMENT REPORT messages sent = %d",
        ueL3->stat.numMeasReportSent);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    sprintf(
        buf,
        "Number of CPICH MEASUREMENT reports received from PHY = %d",
        ueL3->stat.numCpichMeasFromPhyRcvd);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    // Handover
#if 0
    // disabled for now, unable to measure them
    sprintf(
        buf,
        "Number of Hard Handover Performed = %d",
        ueL3->stat.numHardHoPeformed);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    sprintf(
        buf,
        "Number of Soft Handover Performed = %d",
        ueL3->stat.numSoftHoPerformed);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    sprintf(
        buf,
        "Number of Softer Handover Performed = %d",
        ueL3->stat.numSofterHoPerformed);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    sprintf(
        buf,
        "Number of Intra-Cell Softer Handover Performed = %d",
        ueL3->stat.numIntraCellShoPerformed);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    sprintf(
        buf,
        "Number of Inter-Cell Intra-RNC Soft Handover Performed = %d",
        ueL3->stat.numInterCellIntraRncShoPerformed);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    sprintf(
        buf,
        "Number of Inter-RNC Intra-SGSN Soft Handover Performed = %d",
        ueL3->stat.numInterRncIntraSgsnShoPerformed);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    sprintf(
        buf,
        "Number of Inter-SGSN Soft Handover Performed = %d",
        ueL3->stat.numInterSgsnShoPerformed);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);
#endif

    sprintf(
        buf,
        "Number of ACTIVE SET UPDATE messages received = %d",
        ueL3->stat.numActiveSetSetupRcvd);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    sprintf(
        buf,
        "Number of ACTIVE SET UPDATE COMPLETE messages sent = %d",
        ueL3->stat.numActiveSetupCompSent);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    // packet
#if 0
   // disabled for now
    sprintf(
        buf,
        "Number of NAS Messages Sent to Layer2 = %d",
        ueL3->stat.numNasMsgSentToL2);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    sprintf(
        buf,
        "Number of Data Packet From Upper Layer = %d",
        ueL3->stat.numDataPacketFromUpper);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);
#endif

    // --- PS DOMAIN
    sprintf(
        buf,
        "Number of PS domain data packets received from upper layer = %d",
        ueL3->stat.numPsDataPacketFromUpper);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    sprintf(
        buf,
       "Number of PS domain data packets dropped (unsupported format) = %d",
        ueL3->stat.numDataPacketDroppedUnsupportedFormat);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);
#if 0
    // disabled for now
    sprintf(
        buf,
        "Number of PS domain data packets enqueued = %d",
        ueL3->stat.numPsDataPacketFromUpperEnQueue);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    sprintf(
        buf,
        "Number of PS domain data packets dequeued = %d",
        ueL3->stat.numPsDataPacketFromUpperDeQueue);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);
#endif

    sprintf(
        buf,
        "Number of PS domain data packets sent to lower layer = %d",
        ueL3->stat.numPsDataPacketSentToL2);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    sprintf(
        buf,
        "Number of PS domain data packet dropped = %d",
        ueL3->stat.numPsPktDroppedNoResource);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

#if 0
    // disabled for now
    sprintf(
        buf,
        "Number of data packets received from layer 2 = %d",
        ueL3->stat.numDataPacketRcvdFromL2);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);
#endif

    sprintf(
        buf,
        "Number of PS domain data packets received from lower layer = %d",
        ueL3->stat.numPsDataPacketRcvdFromL2);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    // --- CS DOMAIN
    sprintf(
        buf,
        "Number of CS domain data packets received from upper layer = %d",
        ueL3->stat.numCsDataPacketFromUpper);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    sprintf(
        buf,
        "Number of CS domain data packets sent to lower layer = %d",
        ueL3->stat.numCsDataPacketSentToL2);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    sprintf(
        buf,
        "Number of CS domain data packet dropped = %d",
        ueL3->stat.numCsPktDroppedNoResource);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    sprintf(
        buf,
        "Number of CS domain data packets received from lower layer = %d",
        ueL3->stat.numCsDataPacketRcvdFromL2);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    sprintf(
        buf,
        "Number of packets (data & ctrl) received from lower layer = %d",
        ueL3->stat.numPacketRcvdFromL2);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    // Applicaiton
    //--- CS

    //----PS
    //--MT
    // PS data
    sprintf(
        buf,
        "Number of mobile terminated flows requested = %d",
        ueL3->stat.numMtAppRcvdFromNetwork);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    sprintf(
        buf,
        "Number of mobile terminated flows admitted = %d",
        ueL3->stat.numMtAppInited);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    sprintf(
        buf,
        "Number of mobile terminated flows rejected = %d",
        ueL3->stat.numMtAppRejected);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    sprintf(
        buf,
        "Number of mobile terminated flows dropped = %d",
        ueL3->stat.numMtAppDropped);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    sprintf(
        buf,
        "Number of mobile terminated flows completed = %d",
        ueL3->stat.numMtAppSucEnded);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    if (umtsL3->printDetailedStat)
    {
        //---- Conversational
        sprintf(
            buf,
            "Number of mobile terminated CS domain flows requested  = %d",
            ueL3->stat.numMtCsAppRcvdFromNetwork);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of mobile terminated CS domain flows admitted = %d",
            ueL3->stat.numMtCsAppInited);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of mobile terminated CS domain flows rejected = %d",
            ueL3->stat.numMtCsAppRejected);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of mobile terminated CS domain flows dropped = %d",
            ueL3->stat.numMtCsAppDropped);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of mobile terminated CS domain flows completed = %d",
            ueL3->stat.numMtCsAppSucEnded);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        //---- Conversational
        sprintf(
            buf,
         "Number of mobile terminated conversational flows requested  = %d",
            ueL3->stat.numMtConversationalAppRcvdFromNetwork);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
           "Number of mobile terminated conversational flows admitted = %d",
            ueL3->stat.numMtConversationalAppInited);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
           "Number of mobile terminated conversational flows rejected = %d",
            ueL3->stat.numMtConversationalAppRejected);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of mobile terminated conversational flows dropped = %d",
            ueL3->stat.numMtConversationalAppDropped);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
          "Number of mobile terminated conversational flows completed = %d",
            ueL3->stat.numMtConversationalAppSucEnded);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);
        //---- Streaming
        sprintf(
            buf,
            "Number of mobile terminated streaming flows requested = %d",
            ueL3->stat.numMtStreamingAppRcvdFromNetwork);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of mobile terminated streaming flows admitted = %d",
            ueL3->stat.numMtStreamingAppInited);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of mobile terminated streaming flows rejected = %d",
            ueL3->stat.numMtStreamingAppRejected);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of mobile terminated streaming flows dropped = %d",
            ueL3->stat.numMtStreamingAppDropped);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of mobile terminated streaming flows completed = %d",
            ueL3->stat.numMtStreamingAppSucEnded);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);
        //---- Interactive
        sprintf(
            buf,
            "Number of mobile terminated interactive flows requested = %d",
            ueL3->stat.numMtInteractiveAppRcvdFromNetwork);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of mobile terminated interactive flows admitted = %d",
            ueL3->stat.numMtInteractiveAppInited);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of mobile terminated interactive flows rejected = %d",
            ueL3->stat.numMtInteractiveAppRejected);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of mobile terminated interactive flows dropped = %d",
            ueL3->stat.numMtInteractiveAppDropped);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of mobile terminated interactive flows completed = %d",
            ueL3->stat.numMtInteractiveAppSucEnded);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);
        //---- Background
        sprintf(
            buf,
            "Number of mobile terminated background flows requested = %d",
            ueL3->stat.numMtBackgroundAppRcvdFromNetwork);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of mobile terminated background flows admitted = %d",
            ueL3->stat.numMtBackgroundAppInited);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of mobile terminated background flows rejected = %d",
            ueL3->stat.numMtBackgroundAppRejected);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of mobile terminated background flows dropped = %d",
            ueL3->stat.numMtBackgroundAppDropped);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of mobile terminated background flows completed = %d",
            ueL3->stat.numMtBackgroundAppSucEnded);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);
    }

    //--MO
    // PS data
    sprintf(
        buf,
        "Number of mobile originated flows requested = %d",
        ueL3->stat.numMoAppRcvdFromUpper);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    sprintf(
        buf,
        "Number of mobile originated flows admitted = %d",
        ueL3->stat.numMoAppInited);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    sprintf(
        buf,
        "Number of mobile originated flows rejected = %d",
        ueL3->stat.numMoAppRejected);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    sprintf(
        buf,
        "Number of mobile originated flows dropped = %d",
        ueL3->stat.numMoAppDropped);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    sprintf(
        buf,
        "Number of mobile originated flows completed = %d",
        ueL3->stat.numMoAppSucEnded);
    IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

    if (umtsL3->printDetailedStat)
    {
        //---- CS domain
        sprintf(
            buf,
            "Number of mobile originated CS domain flows requested = %d",
            ueL3->stat.numMoCsAppRcvdFromUpper);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of mobile originated CS domain flows admitted = %d",
            ueL3->stat.numMoCsAppInited);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of mobile originated CS domain flows rejected = %d",
            ueL3->stat.numMoCsAppRejected);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of mobile originated CS domain flows dropped = %d",
            ueL3->stat.numMoCsAppDropped);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of mobile originated CS domain flows completed = %d",
            ueL3->stat.numMoCsAppSucEnded);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        //---- Conversational
        sprintf(
            buf,
         "Number of mobile originated conversational flows requested = %d",
            ueL3->stat.numMoConversationalAppRcvdFromUpper);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
          "Number of mobile originated conversational flows admitted = %d",
            ueL3->stat.numMoConversationalAppInited);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
          "Number of mobile originated conversational flows rejected = %d",
            ueL3->stat.numMoConversationalAppRejected);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
          "Number of mobile originated conversational flows dropped = %d",
            ueL3->stat.numMoConversationalAppDropped);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
          "Number of mobile originated conversational flows completed = %d",
            ueL3->stat.numMoConversationalAppSucEnded);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);
        //---- Streaming
        sprintf(
            buf,
            "Number of mobile originated streaming flows requested = %d",
            ueL3->stat.numMoStreamingAppRcvdFromUpper);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of mobile originated streaming flows admitted = %d",
            ueL3->stat.numMoStreamingAppInited);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of mobile originated streaming flows rejected = %d",
            ueL3->stat.numMoStreamingAppRejected);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of mobile originated streaming flows dropped = %d",
            ueL3->stat.numMoStreamingAppDropped);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of mobile originated streaming flows completed = %d",
            ueL3->stat.numMoStreamingAppSucEnded);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);
        //---- Interactive
        sprintf(
            buf,
            "Number of mobile originated interactive flows requested = %d",
            ueL3->stat.numMoInteractiveAppRcvdFromUpper);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of mobile originated interactive flows admitted = %d",
            ueL3->stat.numMoInteractiveAppInited);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of mobile originated interactive flows rejected = %d",
            ueL3->stat.numMoInteractiveAppRejected);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of mobile originated interactive flows dropped = %d",
            ueL3->stat.numMoInteractiveAppDropped);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of mobile originated interactive flows completed = %d",
            ueL3->stat.numMoInteractiveAppSucEnded);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);
        //---- Background
        sprintf(
            buf,
            "Number of mobile originated background flows requested = %d",
            ueL3->stat.numMoBackgroundAppRcvdFromUpper);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of mobile originated background flows admitted = %d",
            ueL3->stat.numMoBackgroundAppInited);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of mobile originated background flows rejected = %d",
            ueL3->stat.numMoBackgroundAppRejected);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of mobile originated background flows dropped = %d",
            ueL3->stat.numMoBackgroundAppDropped);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);

        sprintf(
            buf,
            "Number of mobile originated background flows completed = %d",
            ueL3->stat.numMoBackgroundAppSucEnded);
        IO_PrintStat(node, "Layer3", "UMTS UE", ANY_DEST, -1, buf);
    }

    return;
}

//-------------------------------------------------------------------------
// BUild Msg block
//-------------------------------------------------------------------------
//
// Build MM/GMM Msg
//
// /**
// FUNCTION   :: UmtsMmUeSendLocationUpdateRequest
// LAYER      :: Layer3 MM
// PURPOSE    :: Send a combined location update
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data*   : Pointer to L3 data
// + locUpdateType    : UmtsLocUpdateType : Location update type
// RETURN     :: void : NULL
// **/
static
void UmtsMmUeSendLocationUpdateRequest(Node* node,
                                       UmtsLayer3Data* umtsL3,
                                       UmtsLocUpdateType locUpdate)
{
    UmtsLayer3Ue *ueL3 = (UmtsLayer3Ue*) umtsL3->dataPtr;
    char pktBuff[UMTS_MAX_PACKET_BUFFER_SIZE];
    UmtsLayer3RoutingUpdateRequest *raReqMsg;
    UmtsRoutingAreaId rai;
    int index = 0;

    if (DEBUG_MM)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_MM);
        printf("\n\tsend loc up with type %d to NW\n", locUpdate);
    }
    memset(pktBuff, 0, UMTS_MAX_PACKET_BUFFER_SIZE);
    raReqMsg = (UmtsLayer3RoutingUpdateRequest *) &(pktBuff[0]);
    index += sizeof(UmtsLayer3RoutingUpdateRequest);

    raReqMsg->updateType = locUpdate;
    ueL3->imsi->getMCCMNC(rai.mccMnc);
    memcpy(&(rai.lac[0]), &(ueL3->rrcData->selectedCellId), sizeof(UInt32));
    memcpy(&(raReqMsg->oldRai), &rai, sizeof(rai));

    index += ueL3->imsi->getCompactIMSI(&(pktBuff[index]));

    UmtsLayer3UeSendNasMsg(
        node,
        umtsL3,
        pktBuff,
        index,
        (char)UMTS_LAYER3_PD_MM,
        (char)UMTS_MM_MSG_TYPE_LOCATION_UPDATING_REQUEST,
        0,
        UMTS_LAYER3_CN_DOMAIN_CS);
}

// /**
// FUNCTION   :: UmtsLayer3UeSendCmServReq
// LAYER      :: Layer3 GMM
// PURPOSE    :: Send a CM SERVICE REQUEST to initiate MM connection
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data*   : Pointer to L3 data
// + ueMm             : UmtsMmMsData*     : Pointer to the UE MM data
// + pd               : UmtsLayer3PD      : Protocol discriminator
// + ti               : unsigned char     : transaction Id
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeSendCmServReq(
        Node* node,
        UmtsLayer3Data* umtsL3,
        UmtsMmMsData* ueMm,
        UmtsLayer3PD pd,
        unsigned char ti)
{
    char pktBuf[sizeof(pd)];
    memcpy(&pktBuf, &pd, sizeof(pd));

    UmtsLayer3UeSendNasMsg(
        node,
        umtsL3,
        pktBuf,
        sizeof(pd),
        (char)UMTS_LAYER3_PD_MM,
        (char)UMTS_MM_MSG_TYPE_CM_SERVICE_REQUEST,
        ti,
        UMTS_LAYER3_CN_DOMAIN_CS);

    //initiate timer T3230
    ERROR_Assert(ueMm->T3230 == NULL, "Inconsistent MM state");
    ueMm->T3230 = UmtsLayer3SetTimer(
                    node,
                    umtsL3,
                    UMTS_LAYER3_TIMER_T3230,
                    UMTS_LAYER3_TIMER_T3230_VAL,
                    NULL,
                    NULL,
                    0);
}

// /**
// FUNCTION   :: UmtsLayer3UeSendPagingResp
// LAYER      :: Layer3 GMM
// PURPOSE    :: Send a paging response message
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data*   : Pointer to L3 data
// + ueL3             : UmtsLayer3Ue *    : Pointer to l3 Ue data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeSendPagingResp(
         Node* node,
         UmtsLayer3Data* umtsL3,
         UmtsLayer3Ue* ueL3)
{
    UmtsLayer3UeSendNasMsg(
        node,
        umtsL3,
        NULL,
        0,
        (char)UMTS_LAYER3_PD_MM,
        (char)UMTS_MM_MSG_TYPE_PAGING_RESPONSE,
        0,
        UMTS_LAYER3_CN_DOMAIN_CS);
}

// /**
// FUNCTION   :: UmtsGMmUeSendAttachRequest
// LAYER      :: Layer3 GMM
// PURPOSE    :: Send a Attach request
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data*   : Pointer to L3 data
// + attachType       : UmtsAttachType    : attach request type
// RETURN     :: void : NULL
// **/
static
void UmtsGMmUeSendAttachRequest(Node* node,
                               UmtsLayer3Data* umtsL3,
                               UmtsAttachType attachType)
{
    UmtsLayer3Ue *ueL3 = (UmtsLayer3Ue*) umtsL3->dataPtr;
    char pktBuff[UMTS_MAX_PACKET_BUFFER_SIZE];
    UmtsLayer3AttachRequest *attachReqMsg;
    int index = 0;

    memset(pktBuff, 0, UMTS_MAX_PACKET_BUFFER_SIZE);
    attachReqMsg = (UmtsLayer3AttachRequest *) &(pktBuff[0]);
    index += sizeof(UmtsLayer3AttachRequest);

    // fill the contents
    attachReqMsg->attachType = (unsigned char)attachType;

    // skip the idnetity type an lenght
    index += ueL3->imsi->getCompactIMSI(&(pktBuff[index]));

    UmtsLayer3UeSendNasMsg(
        node,
        umtsL3,
        pktBuff,
        index,
        (char)UMTS_LAYER3_PD_GMM,
        (char)UMTS_GMM_MSG_TYPE_ATTACH_REQUEST,
        0,
        UMTS_LAYER3_CN_DOMAIN_PS);
}

// /**
// FUNCTION   :: UmtsGMmUeSendAttachCompl
// LAYER      :: Layer3 GMM
// PURPOSE    :: Send a Attach request
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data*   : Pointer to L3 data
// + followOn         : BOOL              : follow on service or not
// RETURN     :: void : NULL
// **/
static
void UmtsGMmUeSendAttachCompl(Node* node,
                               UmtsLayer3Data* umtsL3,
                               BOOL followOn)
{
    char pktBuff;

    if (followOn)
    {
        pktBuff = 1;
    }
    else
    {
        pktBuff = 0;
    }

    UmtsLayer3UeSendNasMsg(
        node,
        umtsL3,
        &pktBuff,
        sizeof(char),
        (char)UMTS_LAYER3_PD_GMM,
        (char)UMTS_GMM_MSG_TYPE_ATTACH_COMPLETE,
        0,
        UMTS_LAYER3_CN_DOMAIN_PS);
}

// FUNCTION   :: UmtsLayer3UeSendServiceRequest
// LAYER      :: Layer3 GMM
// PURPOSE    :: Send a Attach request
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data*   : Pointer to L3 data
// + ueL3             : UmtsLayer3Ue *    : UE Layer3 data
// + serviceType      : unsigned char     : service type
// + trasnId          : unsigned char     : transaction Id
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeSendServiceRequest(Node* node,
                                    UmtsLayer3Data* umtsL3,
                                    UmtsLayer3Ue *ueL3,
                                    unsigned char serviceType,
                                    unsigned char transId)
{
    char pktBuff[UMTS_MAX_PACKET_BUFFER_SIZE];
    UmtsLayer3ServiceRequest *reqMsg;
    //UmtsRoutingAreaId rai;
    int index = 0;

    if (DEBUG_GMM)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_GMM);
        printf("\n\tSend service request for TransId %d\n", transId);
    }
    memset(pktBuff, 0, UMTS_MAX_PACKET_BUFFER_SIZE);
    reqMsg = (UmtsLayer3ServiceRequest *) &(pktBuff[0]);
    index += sizeof(UmtsLayer3ServiceRequest);

    reqMsg->serviceType = serviceType;
    UmtsLayer3UeSendNasMsg(
        node,
        umtsL3,
        pktBuff,
        index,
        (char)UMTS_LAYER3_PD_GMM,
        (char)UMTS_GMM_MSG_TYPE_SERVICE_REQUEST,
        transId,
        UMTS_LAYER3_CN_DOMAIN_PS);
}

// /**
// FUNCTION   :: UmtsUeInitLocationUpdate
// LAYER      :: Layer3 MM
// PURPOSE    :: Init Loction Update
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data*   : Pointer to L3 data
// + ueL3             : UmtsLayer3Ue *    : Pointer to l3 Ue data
// + locUpdateType    : UmtsLocUpdateType : Location update type
// RETURN     :: void : NULL
// **/
static
void UmtsMmUeInitLocationUpdate(
                         Node* node,
                         UmtsLayer3Data* umtsL3,
                         UmtsLayer3Ue* ueL3,
                         UmtsLocUpdateType locUpdate)
{

    // if no RRC connection exist
    // send RRC connection est req w/ estCasue
    // UMTS_RRC_ESTCAUSE_REGISTRATION,
    // UMTS_RRC_ESTCAUSE_DETACH
    // update mm main state/update substate
    UmtsMmMsData *ueMM = ueL3->mmData;

    ueMM->mainState = UMTS_MM_MS_MM_IDLE;
    ueMM->idleSubState = UMTS_MM_MS_IDLE_ATTEMPTING_TO_UPDATE;

    BOOL rrcConnected;
    BOOL procInit = UmtsLayer3UeInitRrcConnReq(
                        node,
                        umtsL3,
                        UMTS_RRC_ESTCAUSE_REGISTRATION,
                        UMTS_LAYER3_CN_DOMAIN_CS,
                        &rrcConnected);
    if (rrcConnected)
    {
        ueMM->mainState = UMTS_MM_MS_LOCATION_UPDATING_INITIATED;
        //TODO
        //Build location update message
        //Send it to the network using Init Direct Transfer
        UmtsMmUeSendLocationUpdateRequest(
                node,
                umtsL3,
                locUpdate);

        // update stat
        ueL3->stat.numLocUpReqSent ++;

    }
    else if (procInit)
    {
        ueMM->mainState =
            UMTS_MM_MS_WAIT_FOR_RR_CONNECTION_LOCATION_UPDATING;
        ueMM->nextLocUpType = locUpdate;

    }
    else
    {
        ueMM->mainState = UMTS_MM_MS_MM_IDLE;
        ueMM->idleSubState = UMTS_MM_MS_IDLE_NO_CELL_AVAILABLE;
    }
}

// /**
// FUNCTION   :: UmtsLayer3UeInitPagingResp
// LAYER      :: Layer3 GMM
// PURPOSE    :: Init sending paging response
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data*   : Pointer to L3 data
// + ueL3             : UmtsLayer3Ue *    : Pointer to l3 Ue data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeInitPagingResp(
         Node* node,
         UmtsLayer3Data* umtsL3,
         UmtsLayer3Ue* ueL3)
{
    BOOL rrcConnected, procInit;

    procInit = UmtsLayer3UeInitRrcConnReq(
                    node,
                    umtsL3,
                    UMTS_RRC_ESTCAUSE_TERMINATING_UNKNOWN,
                    UMTS_LAYER3_CN_DOMAIN_CS,
                    &rrcConnected);
    if (rrcConnected)
    {
        UmtsLayer3UeSendPagingResp(node, umtsL3, ueL3);
    }
}

// /**
// FUNCTION   :: UmtsLayer3UeInitGmmAttachProc
// LAYER      :: Layer3 GMM
// PURPOSE    :: Handle GMM state update
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : interface index
// + umtsL3           : UmtsLayer3Data*   : Pointer to L3 data
// + ueL3             : UmtsLayer3Ue *    : Pointer to l3 Ue data
// + followUp         : BOOL              : Follow up service or not
// + combineAttach    : BOLL              : Combined attach or not
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeInitGmmAttachProc(
         Node* node,
         UmtsLayer3Data* umtsL3,
         UmtsLayer3Ue* ueL3,
         BOOL followUp,
         BOOL combineAttach)
{
    if (UmtsGMmUeGetGMmState(ueL3) == UMTS_GMM_MS_REGISTERED_INITIATED)
    {
        // just wait
        return;
    }
    else if (UmtsGMmUeGetGMmState(ueL3) ==
        UMTS_GMM_MS_DEREGISTERED_INITIATED)
    {
        // TODO: need some mechnism to reinitaite the attach proce
        // after detach finished if it is neccessary
        return;
    }
    else
    {
        UmtsAttachType attachType;

        if (!followUp && !combineAttach)
        {
            attachType = UMTS_GPRS_ATTACH_ONLY_NO_FOLLOW_ON;
        }
        else if (followUp && !combineAttach)
        {
            attachType = UMTS_GPRS_ATTACH_ONLY_FOLLOW_ON;
        }
        else if (!followUp && combineAttach)
        {
            attachType = UMTS_COMBINED_ATTACH_NO_FOLLOW_ON;
        }
        else
        {
            attachType = UMTS_COMBINED_ATTACH_FOLLOW_ON;
        }
        // to see if RRC is exsit for this UE
        if (ueL3->rrcData->state == UMTS_RRC_STATE_CONNECTED &&
            ueL3->rrcData->subState == UMTS_RRC_SUBSTATE_CELL_DCH)
        {

            // init attach attempt counter
            ueL3->gmmData->attachAtmpCount = 0;

            // send the attach request
            UmtsGMmUeSendAttachRequest(node, umtsL3, attachType);
            ueL3->stat.numAttachReqSent ++;

            // start timer T3310
            if (ueL3->gmmData->T3310)
            {
                MESSAGE_CancelSelfMsg(node, ueL3->gmmData->T3310);
                ueL3->gmmData->T3310 = NULL;
            }

            ueL3->gmmData->T3310 =
                UmtsLayer3SetTimer(
                    node, umtsL3, UMTS_LAYER3_TIMER_T3310,
                    UMTS_LAYER3_TIMER_T3310_VAL, NULL,
                    (void*)&attachType, sizeof(UmtsAttachType));

            // GMM enter registed initiated state
            UmtsGMmUeSetGMmState(ueL3, UMTS_GMM_MS_REGISTERED_INITIATED);
        }
        else if (ueL3->rrcData->state == UMTS_RRC_STATE_IDLE)
        {
            BOOL rrcConnected;
            BOOL procInit = UmtsLayer3UeInitRrcConnReq(
                                node,
                                umtsL3,
                                UMTS_RRC_ESTCAUSE_REGISTRATION,
                                UMTS_LAYER3_CN_DOMAIN_PS,
                                &rrcConnected);
            if (rrcConnected)
            {
                // do nothing
            }
            else if (procInit)
            {
                ueL3->gmmData->deRegSubState =
                    UMTS_GMM_MS_DEREGISTERED_ATTACH_NEEDED;
                ueL3->gmmData->nextAttachType = attachType;
            }
            else
            {
                // TODO
            }
        }
        // call initRrcConnect
        // move to init  register
        // update GMM deregisted accordingly
        // if rrc needs to be setup
        //     when rrc hasfinished, in that handling function call
        //     build Attach Setup and passs it to the RLC, start timer T3310
        // if rrc has alreay been build up
        //     build attach setup and pass it to the RLC, start tiemr T3310
        // in either case, when Attach request is accpeted, in that handling
        // funciton, check if the follow on proceed inidcation field to
        // see if any PS applicaitons in the appList,
        // and if there is any check if any packets in queue,
        // and send them to RLC
    }
}

// /**
// FUNCTION   :: UmtsLayer3UeAbortServiceRequest
// LAYER      :: Layer3 GMM
// PURPOSE    :: Handle GMM state update
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data*   : Pointer to L3 data
// + ueL3             : UmtsLayer3Ue *    : Pointer to l3 Ue data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeAbortServiceRequest(
         Node* node,
         UmtsLayer3Data* umtsL3,
         UmtsLayer3Ue* ueL3)
{
    UmtsGMmMsData* ueGmm = ueL3->gmmData;

    ueGmm->mainState = UMTS_GMM_MS_REGISTERED;
    ueL3->gmmData->regSubState = UMTS_GMM_MS_REGISTERED_NORMAL_SERVICE;
    ueGmm->servReqWaitForRrc = FALSE;

    std::list<UmtsLayer3UeAppInfo*>::iterator it;
    for (it = ueL3->appInfoList->begin();
         it != ueL3->appInfoList->end();
         ++it)
    {
        if ((*it)->classifierInfo->domainInfo
                    == UMTS_LAYER3_CN_DOMAIN_PS
             && (*it)->cmData.smData->smState ==
                    UMTS_SM_MS_PDP_INACTIVE
             && (*it)->srcDestType == UMTS_FLOW_MOBILE_ORIGINATED)
        {
            while (!(*it)->inPacketBuf->empty())
            {
                MESSAGE_Free(node, (*it)->inPacketBuf->front());
                (*it)->inPacketBuf->pop();
                ueL3->stat.numPsDataPacketFromUpperDeQueue ++;
                ueL3->stat.numPsPktDroppedNoResource ++;
            }
            (*it)->pktBufSize = 0;
        } // if ((*it))
    } // for (it)
}

// /**
// FUNCTION   :: UmtsLayer3UeInitServiceRequest
// LAYER      :: Layer3 GMM
// PURPOSE    :: Handle GMM state update
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : interface index
// + umtsL3           : UmtsLayer3Data*   : Pointer to L3 data
// + ueL3             : UmtsLayer3Ue *    : Pointer to l3 Ue data
// + estCause         : UmtsRrcEstCause estCause : Est. casue
// + servicetType     : unsigned char     : service tytpe
// + transId          : unsigned char     : transaction id
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeInitServiceRequest(
         Node* node,
         UmtsLayer3Data* umtsL3,
         UmtsLayer3Ue* ueL3,
         UmtsRrcEstCause estCause,
         unsigned char serviceType,
         unsigned char transId)
{
    if (ueL3->gmmData->mainState == UMTS_GMM_MS_SERVICE_REQUEST_INITIATED)
    {
        return;
    }

    BOOL rrcConnected;

    BOOL procInit = UmtsLayer3UeInitRrcConnReq(
                        node,
                        umtsL3,
                        estCause,
                        UMTS_LAYER3_CN_DOMAIN_PS,
                        &rrcConnected);
    if (rrcConnected)
    {
        //send serviceRequest
        UmtsLayer3UeSendServiceRequest(
            node, umtsL3, ueL3, serviceType, transId);

        // update stat
        ueL3->stat.numServiceReqSent ++;

        // start timer T3317
        if (ueL3->gmmData->T3317)
        {
            MESSAGE_CancelSelfMsg(node, ueL3->gmmData->T3317);
            ueL3->gmmData->T3317 = NULL;
        }
        ueL3->gmmData->T3317 =
             UmtsLayer3SetTimer(
                    node, umtsL3, UMTS_LAYER3_TIMER_T3317,
                    UMTS_LAYER3_TIMER_T3317_VAL, NULL,
                    NULL, 0);
        ueL3->gmmData->mainState = UMTS_GMM_MS_SERVICE_REQUEST_INITIATED;
        ueL3->gmmData->servReqWaitForRrc = FALSE;
        ueL3->gmmData->serviceTypeRequested = serviceType;
        ueL3->gmmData->servReqTransId = transId;
    }
    else if (procInit)
    {
        ueL3->gmmData->mainState = UMTS_GMM_MS_SERVICE_REQUEST_INITIATED;
        ueL3->gmmData->servReqWaitForRrc = TRUE;
        ueL3->gmmData->serviceTypeRequested = serviceType;
        ueL3->gmmData->servReqTransId = transId;
    }
    else
    {
        UmtsLayer3UeAbortServiceRequest(node, umtsL3, ueL3);
    }
}

// /**
// FUNCTION   :: UmtsMmUeUpdateMmState
// LAYER      :: Layer3 MM
// PURPOSE    :: Handle MM state update
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data*   : Pointer to L3 data
// + ueL3             : UmtsLayer3Ue *    : Pointer to l3 Ue data
// + eventType        : UmtsMmStateUpdateEventType : MM event type
// RETURN     :: void : NULL
// **/
static
void UmtsMmUeUpdateMmState(Node* node,
                           UmtsLayer3Data* umtsL3,
                           UmtsLayer3Ue* ueL3,
                           UmtsMmStateUpdateEventType eventType)
{
    if (eventType == UMTS_MM_UPDATE_PowerOn &&
        ueL3->mmData->mainState ==  UMTS_MM_MS_NULL)
    {
        ueL3->mmData->mainState = UMTS_MM_MS_MM_IDLE;
        ueL3->mmData->idleSubState = UMTS_MM_MS_IDLE_PLMN_SEARCH;
        // update stat
        ueL3->stat.numCellSearch ++;
    }
    else if (eventType == UMTS_MM_UPDATE_CellSelected &&
        ueL3->mmData->mainState == UMTS_MM_MS_MM_IDLE /* &&
        ueL3->mmData->idleSubState == UMTS_MM_MS_IDLE_PLMN_SEARCH */)
    {
        if (ueL3->mmData->updateState == UMTS_MM_NOT_UPDATED)
        {
            // update stat
            if (ueL3->locRoutAreaId == UMTS_INVALID_REG_AREA_ID ||
                ueL3->mmData->idleSubState == UMTS_MM_MS_IDLE_PLMN_SEARCH)
            {
                // first time update or secure a new cell
                // after PLMN failure (no suitable cell for a while)
                ueL3->stat.numCellInitSelection ++;
            }
            ueL3->mmData->idleSubState =
                UMTS_MM_MS_IDLE_LOCATION_UPDATE_NEEDED;

            UmtsLocUpdateType locUpType = UMTS_IMSI_ATTACH;

            // call loction update init process
            if (DEBUG_MM || DEBUG_GMM)
            {
                UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_MM);
                printf("init the IMSI ATTACH to cell %d with RegArea %d\n",
                    ueL3->priNodeB->cellId,ueL3->priNodeB->regAreaId);

            }
            UmtsMmUeInitLocationUpdate(node, umtsL3, ueL3, locUpType);
        }
        else if (ueL3->mmData->idleSubState ==
                    UMTS_MM_MS_IDLE_LOCATION_UPDATE_NEEDED)
        {
            UmtsLayer3UeNodebInfo* nodebInfo;


            nodebInfo = UmtsLayer3UeGetNodeBData(
                            node, umtsL3, ueL3,
                            ueL3->rrcData->selectedCellId);

            if (nodebInfo && nodebInfo->regAreaId == ueL3->locRoutAreaId)
            {
                // if not perioidc location update
                // and if last locRear is the same current one
                // skip this update
                ueL3->mmData->idleSubState = UMTS_MM_MS_IDLE_NORMAL_SERVICE;
                ueL3->mmData->updateState = UMTS_MM_UPDATED;

                if (DEBUG_MM || DEBUG_GMM)
                {
                    UmtsLayer3PrintRunTimeInfo(
                        node, UMTS_LAYER3_SUBLAYER_MM);
                    printf("The current RegAreaId is he same as last "
                        "update: cellId %d RegArea %d\n",
                        ueL3->priNodeB->cellId,ueL3->priNodeB->regAreaId);

                }
                return;
            }
            else if (nodebInfo)
            {

                // TODO: compare last update time
                // if timer not expire
                ueL3->mmData->updateState = UMTS_MM_NOT_UPDATED;
                UmtsLocUpdateType locUpType = UMTS_NORMAL_LOCATION_UPDATING;

                // else
                //locUpType = UMTS_PERIODIC_LOCATION_UPDATING;
                // call loction update init process
                UmtsMmUeInitLocationUpdate(node, umtsL3, ueL3, locUpType);
            }
        }
        else
        {
            ueL3->mmData->idleSubState = UMTS_MM_MS_IDLE_NORMAL_SERVICE;
            ueL3->mmData->updateState = UMTS_MM_UPDATED;
        }
    }
    else if (eventType == UMTS_MM_UPDATE_NoCellSelected &&
        ueL3->mmData->mainState == UMTS_MM_MS_MM_IDLE /* &&
        ueL3->mmData->idleSubState == UMTS_MM_MS_IDLE_PLMN_SEARCH*/)
    {
        // do nothing
        ueL3->mmData->idleSubState = UMTS_MM_MS_IDLE_NO_CELL_AVAILABLE;
    }
}

// FUNCTION   :: UmtsLayer3UeReqMmConnEst
// LAYER      :: Layer3 MM
// PURPOSE    :: Request to establish MM connection by CM entities
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data*   : Pointer to L3 data
// + ueL3             : UmtsLayer3Ue *    : Pointer to l3 Ue data
// + pd               : UmtsLayer3PD      : Protocol discrimator
// + ti               : unsigned char     : Transaction ID
// + reqInit          : BOOL*             : Whether MM connection request
//                                          procedure can be initiated
// RETURN     :: BOOL : TRUE if the MM connection is active
// **/
static
BOOL UmtsLayer3UeReqMmConnEst(
         Node* node,
         UmtsLayer3Data* umtsL3,
         UmtsLayer3Ue* ueL3,
         UmtsLayer3PD pd,
         unsigned char ti,
         BOOL* reqInit)
{
    BOOL retVal = FALSE;
    *reqInit = TRUE;
    UmtsMmMsData* ueMm = ueL3->mmData;
    //If the requesting MM connection queue is not empty, insert the
    //request at the end of the queue
    if (!ueMm->rqtingMmConns->empty())
    {
        UmtsLayer3MmConn cmpMmConn = UmtsLayer3MmConn(pd, ti);
        if (ueMm->rqtingMmConns->end() ==
                find_if (ueMm->rqtingMmConns->begin(),
                        ueMm->rqtingMmConns->end(),
                        bind2nd(UmtsPointedItemEqual<UmtsLayer3MmConn>(),
                                &cmpMmConn)))
        {
            ueMm->rqtingMmConns->push_back(new UmtsLayer3MmConn(cmpMmConn));
        }
    }
    else
    {
        //if the MM is not updated, delay the CM service request
        if (ueMm->updateState != UMTS_MM_UPDATED)
        {
            ueMm->rqtingMmConns->push_back(new UmtsLayer3MmConn(pd, ti));
        }
        else if (ueMm->mainState == UMTS_MM_MS_MM_CONNECTION_ACTIVE)
        {
            //If the MM connection is already active, do nothing
            UmtsLayer3MmConn cmpMmConn = UmtsLayer3MmConn(pd, ti);
            if (ueMm->actMmConns->end() !=
                    find_if (ueMm->actMmConns->begin(),
                            ueMm->actMmConns->end(),
                           bind2nd(UmtsPointedItemEqual<UmtsLayer3MmConn>(),
                                    &cmpMmConn)))
            {
                retVal = TRUE;
            }
            //otherwise, send the MM connection request directly
            //since the RR connection is already there.
            else
            {
                UmtsLayer3UeSendCmServReq(node, umtsL3, ueMm, pd, ti);
                ueMm->mainState =
                    UMTS_MM_MS_WAIT_FOR_ADDITIONAL_OUTGOING_MM_CONNECTION;
                ueMm->rqtingMmConns->
                         push_back(new UmtsLayer3MmConn(pd, ti));
            }
        }
        //If the RR connection(CS domain signal connection) is connected,
        //but no active MM connection, send the
        //CM service request imediately
        else if (ueMm->mainState ==
                    UMTS_MM_MS_WAIT_FOR_NETWORK_COMMAND)
        {
            if (ueMm->T3240)
            {
                MESSAGE_CancelSelfMsg(node, ueMm->T3240);
                ueMm->T3240 = NULL;
            }
            UmtsLayer3UeSendCmServReq(node, umtsL3, ueMm, pd, ti);
            ueMm->mainState =
                UMTS_MM_MS_WAIT_FOR_OUTGOING_MM_CONNECTION;
            ueMm->rqtingMmConns->push_back(new UmtsLayer3MmConn(pd, ti));
        }
        //Or if the CS signal connection is not
        //connected and no specific MM
        //procedure(Loc Update) is ongoing,
        //starts to initiate RR connection
        //(CS signal connection)
        else if (ueMm->mainState == UMTS_MM_MS_MM_IDLE)
        {
            BOOL rrcConnected;
            BOOL procInit =
                UmtsLayer3UeInitRrcConnReq(
                            node,
                            umtsL3,
                            UMTS_RRC_ESTCAUSE_ORIGINATING_CONVERSATIONAL,
                            UMTS_LAYER3_CN_DOMAIN_CS,
                            &rrcConnected);
            if (rrcConnected)
            {
                UmtsLayer3UeSendCmServReq(node, umtsL3, ueMm, pd, ti);
                ueMm->mainState =
                    UMTS_MM_MS_WAIT_FOR_OUTGOING_MM_CONNECTION;
                ueMm->rqtingMmConns->push_back(
                                        new UmtsLayer3MmConn(pd, ti));
            }
            else if (procInit == TRUE)
            {
                ueMm->mainState =
                    UMTS_MM_MS_WAIT_FOR_RR_CONNECTION_MM_CONNECTION;
                ueMm->rqtingMmConns->push_back(
                                        new UmtsLayer3MmConn(pd, ti));
            }
            //if RRC CONNECTION REQUEST cannot be initiated for some reason
            //(NO CELL available) drop the CM service request
            else
            {
                *reqInit = FALSE;
            }
        }
        //Otherwise, delay the CM service request
        else
        {
            ueMm->rqtingMmConns->push_back(new UmtsLayer3MmConn(pd, ti));
        }
    }
    return retVal;
}

// FUNCTION   :: UmtsLayer3UeReleaseMmConn
// LAYER      :: Layer3 MM
// PURPOSE    :: Request to release MM connection by CM entities
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data*   : Pointer to L3 data
// + ueL3             : UmtsLayer3Ue *    : Pointer to l3 Ue data
// + pd               : UmtsLayer3PD      : Protocol discrimator
// + ti               : unsigned char     : Transaction ID
// RETURN     :: NULL
// **/
static
void UmtsLayer3UeReleaseMmConn(
         Node* node,
         UmtsLayer3Data* umtsL3,
         UmtsLayer3Ue* ueL3,
         UmtsLayer3PD pd,
         unsigned char ti)
{
    UmtsMmMsData* ueMm = ueL3->mmData;
    UmtsLayer3MmConn cmpMmConn = UmtsLayer3MmConn(pd, ti);

    if (ueMm->mainState != UMTS_MM_MS_MM_CONNECTION_ACTIVE &&
            ueMm->mainState !=
            UMTS_MM_MS_WAIT_FOR_ADDITIONAL_OUTGOING_MM_CONNECTION)
    {
        return;
    }
    UmtsMmConnCont::iterator mmConnIter =
            find_if (ueMm->actMmConns->begin(),
                    ueMm->actMmConns->end(),
                    bind2nd(UmtsPointedItemEqual<UmtsLayer3MmConn>(),
                            &cmpMmConn));
    if (ueMm->actMmConns->end() != mmConnIter)
    {
        delete *mmConnIter;
        ueMm->actMmConns->erase(mmConnIter);
        if (ueMm->actMmConns->empty())
        {
            if (ueMm->mainState == UMTS_MM_MS_MM_CONNECTION_ACTIVE)
            {
                ueMm->mainState = UMTS_MM_MS_WAIT_FOR_NETWORK_COMMAND;
                ERROR_Assert(ueMm->T3240 == NULL, "Inconsistent MM state");
                ueMm->T3240 = UmtsLayer3SetTimer(
                                node,
                                umtsL3,
                                UMTS_LAYER3_TIMER_T3240,
                                UMTS_LAYER3_TIMER_T3240_VAL,
                                NULL,
                                NULL,
                                0);
            }
            else if (ueMm->mainState ==
                UMTS_MM_MS_WAIT_FOR_ADDITIONAL_OUTGOING_MM_CONNECTION)
            {
                ERROR_Assert(!ueMm->rqtingMmConns->empty(),
                    "UmtsLayer3UeReleaseMmConn, invalid CC state.");
                ueMm->mainState = UMTS_MM_MS_WAIT_FOR_OUTGOING_MM_CONNECTION;
            } // if (ueMm->mainState)
        } // if (ueMm->actMmConns->empty())
    } //  if (ueMm->actMmConns->end() != mmConnIter)
}

///****************************************************************
// Build CC Msg
///****************************************************************
// /**
// FUNCTION   :: UmtsLayer3UeSendCallSetup
// LAYER      :: Layer 3
// PURPOSE    :: Send mobile originated call setup
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data*  : Pointer to L3 data
// + ti        : char             : transaction Id
// + ueCC      : UmtsCcMsData*    : UC CC data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeSendCallSetup(
      Node* node,
      UmtsLayer3Data* umtsL3,
      char ti,
      UmtsCcMsData* ueCc)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*)(umtsL3->dataPtr);

    char packet[50];
    int  index = 0;
    memcpy(&packet[index],
           &(ueCc->peerAddr),
           sizeof(ueCc->peerAddr));
    index += sizeof(ueCc->peerAddr);
    memcpy(&packet[index],
           &(ueCc->callAppId),
           sizeof(ueCc->callAppId));
    index += sizeof(ueCc->callAppId);
    memcpy(&(packet[index]),
           &(ueCc->endTime),
           sizeof(ueCc->endTime));
    index += sizeof(ueCc->endTime);
    memcpy(&(packet[index]),
           &(ueCc->avgTalkTime),
           sizeof(ueCc->avgTalkTime));
    index += sizeof(ueCc->avgTalkTime);

    UmtsLayer3UeSendNasMsg(
        node,
        umtsL3,
        packet,
        index,
        (char)UMTS_LAYER3_PD_CC,
        (char)UMTS_CC_MSG_TYPE_SETUP,
        ti,
        UMTS_LAYER3_CN_DOMAIN_CS);

    ueL3->stat.numCcSetupSent++;
}

// /**
// FUNCTION   :: UmtsLayer3UeSendCallConnectAck
// LAYER      :: Layer 3
// PURPOSE    :: Send mobile originated call setup
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data*  : Pointer to L3 data
// + ti        : char             : transaction Id
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeSendCallConnectAck(
      Node* node,
      UmtsLayer3Data* umtsL3,
      char ti)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*)(umtsL3->dataPtr);
    UmtsLayer3UeSendNasMsg(
        node,
        umtsL3,
        NULL,
        0,
        (char)UMTS_LAYER3_PD_CC,
        (char)UMTS_CC_MSG_TYPE_CONNECT_ACKNOWLEDGE,
        ti,
        UMTS_LAYER3_CN_DOMAIN_CS);

    ueL3->stat.numCcConnAckSent++;
}

// /**
// FUNCTION   :: UmtsLayer3UeSendCallConfirm
// LAYER      :: Layer 3
// PURPOSE    :: Send a CALL CONFIRMED message
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data*  : Pointer to L3 data
// + ueFlow    : UmtsLayer3UeAppInfo* : Pointer to the UE application data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeSendCallConfirm(
      Node* node,
      UmtsLayer3Data* umtsL3,
      UmtsLayer3UeAppInfo* ueFlow)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*)(umtsL3->dataPtr);

    UmtsLayer3UeSendNasMsg(
        node,
        umtsL3,
        NULL,
        0,
        (char)UMTS_LAYER3_PD_CC,
        (char)UMTS_CC_MSG_TYPE_CALL_CONFIRMED,
        ueFlow->transId,
        UMTS_LAYER3_CN_DOMAIN_CS);

    ueFlow->cmData.ccData->state
        = UMTS_CC_MS_MOBILE_TERMINATING_CALL_CONFIRMED;

    ueL3->stat.numCcCallConfirmSent++;
}

// /**
// FUNCTION   :: UmtsLayer3UeSendAlertingToMsc
// LAYER      :: Layer 3
// PURPOSE    :: Send an ALERTING message to the MSC
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data*  : Pointer to L3 data
// + ueFlow    : UmtsLayer3UeAppInfo* : Pointer to the UE application data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeSendAlertingToMsc(
          Node* node,
          UmtsLayer3Data* umtsL3,
          UmtsLayer3UeAppInfo* ueFlow)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*)(umtsL3->dataPtr);
    UmtsLayer3UeSendNasMsg(
        node,
        umtsL3,
        NULL,
        0,
        (char)UMTS_LAYER3_PD_CC,
        (char)UMTS_CC_MSG_TYPE_ALERTING,
        ueFlow->transId,
        UMTS_LAYER3_CN_DOMAIN_CS);

    ueFlow->cmData.ccData->state
        = UMTS_CC_MS_CALL_RECEIVED;

    ueL3->stat.numCcAlertingSent++;
}

// /**
// FUNCTION   :: UmtsLayer3UeSendMobTermCallConnect
// LAYER      :: Layer 3
// PURPOSE    :: Send an CONNECT message to the MSC
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data*  : Pointer to L3 data
// + ueFlow    : UmtsLayer3UeAppInfo* : Pointer to the UE application data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeSendMobTermCallConnect(
          Node* node,
          UmtsLayer3Data* umtsL3,
          UmtsLayer3UeAppInfo* ueFlow)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*)(umtsL3->dataPtr);
    UmtsLayer3UeSendNasMsg(
        node,
        umtsL3,
        NULL,
        0,
        (char)UMTS_LAYER3_PD_CC,
        (char)UMTS_CC_MSG_TYPE_CONNECT,
        ueFlow->transId,
        UMTS_LAYER3_CN_DOMAIN_CS);

    ueFlow->cmData.ccData->state
        = UMTS_CC_MS_CONNECT_REQUEST;

    ueL3->stat.numCcConnectSent++;
}

// /**
// FUNCTION   :: UmtsLayer3UeSendCallDisconnect
// LAYER      :: Layer 3
// PURPOSE    :: Send a DISCONNECT message to the MSC
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data*  : Pinter to the UMTS L3 data
// + ueFlow    : UmtsLayer3UeAppInfo* : Pointer to the UE application data
// + clearType : UmtsCallClearingType : Call clear type
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeSendCallDisconnect(
          Node* node,
          UmtsLayer3Data* umtsL3,
          UmtsLayer3UeAppInfo* ueFlow,
          UmtsCallClearingType clearType)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*)(umtsL3->dataPtr);

    char msg[4];
    msg[0] = (char)clearType;
    UmtsLayer3UeSendNasMsg(
        node,
        umtsL3,
        msg,
        sizeof(char),
        (char)UMTS_LAYER3_PD_CC,
        (char)UMTS_CC_MSG_TYPE_DISCONNECT,
        ueFlow->transId,
        UMTS_LAYER3_CN_DOMAIN_CS);

    ueL3->stat.numCcDisconnectSent++;
}

/// /**
// FUNCTION   :: UmtsLayer3UeSendCallRelease
// LAYER      :: Layer 3
// PURPOSE    :: Send a RELEASE message to the MSC
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data*  : Pinter to the UMTS L3 data
// + ueFlow    : UmtsLayer3UeAppInfo* : Pointer to the UE application data
// + clearType : UmtsCallClearingType : Type of clearing action
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeSendCallRelease(
          Node* node,
          UmtsLayer3Data* umtsL3,
          UmtsLayer3UeAppInfo* ueFlow,
          UmtsCallClearingType clearType = UMTS_CALL_CLEAR_END)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*)(umtsL3->dataPtr);
    UmtsLayer3UeSendNasMsg(
        node,
        umtsL3,
        NULL,
        0,
        (char)UMTS_LAYER3_PD_CC,
        (char)UMTS_CC_MSG_TYPE_RELEASE,
        ueFlow->transId,
        UMTS_LAYER3_CN_DOMAIN_CS);

    ueL3->stat.numCcReleaseSent ++;
}

/// /**
// FUNCTION   :: UmtsLayer3UeSendCallReleaseComplete
// LAYER      :: Layer 3
// PURPOSE    :: Send a RELEASE COMPLETE message to the MSC
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + ueL3      : UmtsLayer3Ue *   : Pointer to UMTS UE Layer3 data
// + ueFlow    : UmtsLayer3UeAppInfo* : Pointer to the UE application data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeSendCallReleaseComplete(
          Node* node,
          UmtsLayer3Data* umtsL3,
          UmtsLayer3UeAppInfo* ueFlow)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*)(umtsL3->dataPtr);
    UmtsLayer3UeSendNasMsg(
        node,
        umtsL3,
        NULL,
        0,
        (char)UMTS_LAYER3_PD_CC,
        (char)UMTS_CC_MSG_TYPE_RELEASE_COMPLETE,
        ueFlow->transId,
        UMTS_LAYER3_CN_DOMAIN_CS);

    ueL3->stat.numCcReleaseCompleteSent++;
}

// /**
// FUNCTION   :: UmtsLayer3UeInitMsCallSetup
// LAYER      :: Layer 3
// PURPOSE    :: Initiate mobile originated call setup
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data*  : Pint to the umts L3 data
// + ueL3      : UmtsLayer3Ue *   : Pointer to UMTS UE Layer3 data
// + ueFlow    : UmtsLayer3UeAppInfo* : Pointer to the UE application data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeInitMsCallSetup(
          Node* node,
          UmtsLayer3Data *umtsL3,
          UmtsLayer3Ue* ueL3,
          UmtsLayer3UeAppInfo* ueFlow)
{
    UmtsCcMsData* ueCc = ueFlow->cmData.ccData;
    BOOL reqInit;
    if (UmtsLayer3UeReqMmConnEst(node,
                                 umtsL3,
                                 ueL3,
                                 UMTS_LAYER3_PD_CC,
                                 ueFlow->transId,
                                 &reqInit))
    {
        UmtsLayer3UeSendCallSetup(node,
                                  umtsL3,
                                  ueFlow->transId,
                                  ueCc);
        ueCc->state = UMTS_CC_MS_CALL_INITIATED;
        ueCc->T303 = UmtsLayer3SetTimer(
                            node,
                            umtsL3,
                            UMTS_LAYER3_TIMER_CC_T303,
                            UMTS_LAYER3_TIMER_CC_T303_MS_VAL,
                            NULL,
                            (void*)&(ueFlow->transId),
                            sizeof(ueFlow->transId));
    }
    else if (reqInit)
    {
        ueCc->state = UMTS_CC_MS_MM_CONNECTION_PENDING;
        ueCc->T303 = UmtsLayer3SetTimer(
                            node,
                            umtsL3,
                            UMTS_LAYER3_TIMER_CC_T303,
                            UMTS_LAYER3_TIMER_CC_T303_MS_VAL,
                            NULL,
                            (void*)&(ueFlow->transId),
                            sizeof(ueFlow->transId));
    }
    else
    {
        // report to umtscall that call is rejected
        UmtsLayer3UeNotifyAppCallReject(
            node,
            umtsL3,
            ueFlow,
            APP_UMTS_CALL_REJECT_CAUSE_SYSTEM_BUSY);

        UmtsLayer3UeRemoveAppInfoFromList(
            node,
            ueL3,
            ueFlow->srcDestType,
            ueFlow->transId);
        return;
    }
}

// /**
// FUNCTION   :: UmtsLayer3UeInitCallClearing
// LAYER      :: Layer 3
// PURPOSE    :: Initiate mobile originated call clearing
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data*  : Pinter to the UMTS L3 data
// + ueFlow    : UmtsLayer3UeAppInfo* : Pointer to the UE application data
// + clearType : UmtsCallClearingType : Type of clearing action
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeInitCallClearing(
          Node* node,
          UmtsLayer3Data *umtsL3,
          UmtsLayer3UeAppInfo* ueFlow,
          UmtsCallClearingType clearType)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*)(umtsL3->dataPtr);
    if (ueFlow->srcDestType == UMTS_FLOW_MOBILE_ORIGINATED)
    {
        if (clearType == UMTS_CALL_CLEAR_END)
        {
            ueL3->stat.numMoAppSucEnded ++;
            ueL3->stat.numMoCsAppSucEnded ++;
        }
        else if (clearType == UMTS_CALL_CLEAR_DROP)
        {
            ueL3->stat.numMoAppDropped ++;
            ueL3->stat.numMoCsAppDropped ++;
        }
        else
        {
            ueL3->stat.numMoAppRejected ++;
            ueL3->stat.numMoCsAppRejected ++;
        }
    }
    else
    {
        if (clearType == UMTS_CALL_CLEAR_END)
        {
            ueL3->stat.numMtAppSucEnded ++;
            ueL3->stat.numMtCsAppSucEnded ++;
        }
        else if (clearType == UMTS_CALL_CLEAR_DROP)
        {
            ueL3->stat.numMtAppDropped ++;
            ueL3->stat.numMtCsAppDropped ++;
        }
        else
        {
            ueL3->stat.numMtAppRejected ++;
            ueL3->stat.numMtCsAppRejected ++;
        }
    }

    UmtsCcMsData* ueCc = ueFlow->cmData.ccData;
    if (ueCc->state != UMTS_CC_MS_NULL
        && ueCc->state != UMTS_CC_MS_DISCONNECT_REQUEST
        && ueCc->state != UMTS_CC_MS_RELEASE_REQUEST)
    {
        UmtsLayer3UeStopCcTimers(node, ueL3, ueCc);
        UmtsLayer3UeSendCallDisconnect(node, umtsL3, ueFlow, clearType);
        ueFlow->cmData.ccData->state
            = UMTS_CC_MS_DISCONNECT_REQUEST;
        ueCc->T305 = UmtsLayer3SetTimer(
                            node,
                            umtsL3,
                            UMTS_LAYER3_TIMER_CC_T305,
                            UMTS_LAYER3_TIMER_CC_T305_VAL,
                            NULL,
                            (void*)&(ueFlow->transId),
                            sizeof(ueFlow->transId));
    }
}

///***********************************************************
// Build SM Msg
///***********************************************************
// /**
// FUNCTION   :: UmtsLayer3UeSmSendDeactivatePDPContextRequest
// LAYER      :: Layer 3
// PURPOSE    :: Send PDP activation request
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data*  : Pint to the umts L3 data
// + ueL3      : UmtsLayer3Ue *   : Pointer to UMTS UE Layer3 data
// + srcDestType : UmtsFlowSrcDestType : PDP context source/destination
// + transId   : unsigned char    : transaction Id
// + deactCause : UmtsSmCauseType : deactivete cause
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeSmSendDeactivatePDPContextRequest(
                             Node* node,
                             UmtsLayer3Data *umtsL3,
                             UmtsLayer3Ue* ueL3,
                             UmtsFlowSrcDestType srcDestType,
                             unsigned char transId,
                             UmtsSmCauseType deactCause)
{
    UmtsLayer3UeAppInfo* appInfo;
    char pktBuff[UMTS_MAX_PACKET_BUFFER_SIZE];
    UmtsLayer3DeactivatePDPContextRequest *pdpReqMsg;
    int index = 0;

    if (DEBUG_SM)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_SM);
        printf("\n\tSend a Deactivate PDP CONTEXT REQ for app Trans%d\n",
               transId);
    }
    appInfo = UmtsLayer3UeFindAppInfoFromList(
        node, ueL3, srcDestType, transId);

    memset(pktBuff, 0, UMTS_MAX_PACKET_BUFFER_SIZE);
    pdpReqMsg = (UmtsLayer3DeactivatePDPContextRequest *) &(pktBuff[0]);
    index += sizeof(UmtsLayer3DeactivatePDPContextRequest);

    // fill the contents
    pdpReqMsg->smCause = (unsigned char)deactCause;

    // add extend maxBitRate
    UmtsLayer3UeSendNasMsg(
        node,
        umtsL3,
        pktBuff,
        index,
        (char)UMTS_LAYER3_PD_SM,
        (char)UMTS_SM_MSG_TYPE_DEACTIVATE_PDP_REQUEST,
        transId,
        UMTS_LAYER3_CN_DOMAIN_PS);

    // update app leve stat
    if (deactCause == UMTS_SM_CAUSE_REGUALR_DEACTIVATION)
    {
        if (appInfo->srcDestType == UMTS_FLOW_NETWORK_ORIGINATED)
        {
            ueL3->stat.numMtAppSucEnded ++;
            if (appInfo->qosInfo->trafficClass ==
                    UMTS_QOS_CLASS_CONVERSATIONAL)
            {
                ueL3->stat.numMtConversationalAppSucEnded ++;
            }
            else if (appInfo->qosInfo->trafficClass ==
                UMTS_QOS_CLASS_STREAMING)
            {
                ueL3->stat.numMtStreamingAppSucEnded ++;
            }
            else if (appInfo->qosInfo->trafficClass ==
                UMTS_QOS_CLASS_BACKGROUND)
            {
                ueL3->stat.numMtBackgroundAppSucEnded ++;
            }
            else if (appInfo->qosInfo->trafficClass >
                UMTS_QOS_CLASS_STREAMING &&
                appInfo->qosInfo->trafficClass <
                UMTS_QOS_CLASS_BACKGROUND)
            {
                ueL3->stat.numMtInteractiveAppSucEnded ++;
            }
        }
        else if (appInfo->srcDestType == UMTS_FLOW_MOBILE_ORIGINATED)
        {
            ueL3->stat.numMoAppSucEnded ++;
            if (appInfo->qosInfo->trafficClass ==
                    UMTS_QOS_CLASS_CONVERSATIONAL)
            {
                ueL3->stat.numMoConversationalAppSucEnded ++;
            }
            else if (appInfo->qosInfo->trafficClass ==
                UMTS_QOS_CLASS_STREAMING)
            {
                ueL3->stat.numMoStreamingAppSucEnded ++;
            }
            else if (appInfo->qosInfo->trafficClass ==
                UMTS_QOS_CLASS_BACKGROUND)
            {
                ueL3->stat.numMoBackgroundAppSucEnded ++;
            }
            else if (appInfo->qosInfo->trafficClass >
                UMTS_QOS_CLASS_STREAMING &&
                appInfo->qosInfo->trafficClass <
                UMTS_QOS_CLASS_BACKGROUND)
            {
                ueL3->stat.numMoInteractiveAppSucEnded ++;
            }
        }
    }
    else
    {
        if (appInfo->srcDestType == UMTS_FLOW_NETWORK_ORIGINATED)
        {
            ueL3->stat.numMtAppDropped ++;
            if (appInfo->qosInfo->trafficClass ==
                    UMTS_QOS_CLASS_CONVERSATIONAL)
            {
                ueL3->stat.numMtConversationalAppDropped ++;
            }
            else if (appInfo->qosInfo->trafficClass ==
                UMTS_QOS_CLASS_STREAMING)
            {
                ueL3->stat.numMtStreamingAppDropped ++;
            }
            else if (appInfo->qosInfo->trafficClass ==
                UMTS_QOS_CLASS_BACKGROUND)
            {
                ueL3->stat.numMtBackgroundAppDropped ++;
            }
            else if (appInfo->qosInfo->trafficClass >
                UMTS_QOS_CLASS_STREAMING &&
                appInfo->qosInfo->trafficClass <
                UMTS_QOS_CLASS_BACKGROUND)
            {
                ueL3->stat.numMtInteractiveAppDropped ++;
            }
        }
        else if (appInfo->srcDestType == UMTS_FLOW_MOBILE_ORIGINATED)
        {
            ueL3->stat.numMoAppDropped ++;
            if (appInfo->qosInfo->trafficClass ==
                    UMTS_QOS_CLASS_CONVERSATIONAL)
            {
                ueL3->stat.numMoConversationalAppDropped ++;
            }
            else if (appInfo->qosInfo->trafficClass ==
                UMTS_QOS_CLASS_STREAMING)
            {
                ueL3->stat.numMoStreamingAppDropped ++;
            }
            else if (appInfo->qosInfo->trafficClass ==
                UMTS_QOS_CLASS_BACKGROUND)
            {
                ueL3->stat.numMoBackgroundAppDropped ++;
            }
            else if (appInfo->qosInfo->trafficClass >
                UMTS_QOS_CLASS_STREAMING &&
                appInfo->qosInfo->trafficClass <
                UMTS_QOS_CLASS_BACKGROUND)
            {
                ueL3->stat.numMoInteractiveAppDropped ++;
            }
        } // if (appInfo->srcDestType)
    } // if (deactCause)
}

// /**
// FUNCTION   :: UmtsLayer3UeSmSendDeactivatePDPContextAccept
// LAYER      :: Layer 3
// PURPOSE    :: Send deactivate PDP context accept
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data*  : Pint to the umts L3 data
// + ueL3      : UmtsLayer3Ue *   : Pointer to UMTS UE Layer3 data
// + transId   : unsigned char    : transaction Id
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeSmSendDeactivatePDPContextAccept(
         Node* node,
         UmtsLayer3Data *umtsL3,
         UmtsLayer3Ue* ueL3,
         unsigned char transId)
{
    UmtsLayer3DeactivatePDPContextAccept pdpAcceptMsg;

    if (DEBUG_SM)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_SM);
        printf("\n\tSend a Deactivate PDP CONTEXT Accept for app Trans%d\n",
               transId);
    }

    memset(&pdpAcceptMsg, 0, sizeof(pdpAcceptMsg));

    // fill the contents
    pdpAcceptMsg.transId = transId;

    //  send the message to network
    UmtsLayer3UeSendNasMsg(
        node,
        umtsL3,
        (char *)&pdpAcceptMsg,
        sizeof(pdpAcceptMsg),
        (char)UMTS_LAYER3_PD_SM,
        (char)UMTS_SM_MSG_TYPE_DEACTIVATE_PDP_ACCEPT,
        transId,
        UMTS_LAYER3_CN_DOMAIN_PS);
}

// /**
// FUNCTION   :: UmtsLayer3UeSmSendActivatePDPContextRequest
// LAYER      :: Layer 3
// PURPOSE    :: Send PDP activation request
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + nodeInput : const NodeInput* : Pointer to node input.
// + umtsL3    : UmtsLayer3Data*  : Pint to the umts L3 data
// + ueL3      : UmtsLayer3Ue *   : Pointer to UMTS UE Layer3 data
// + srcDestType : UmtsFlowSrcDestType : PDP context source/destination
// + transId   : unsigned char    : transaction Id
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeSmSendActivatePDPContextRequest(
                             Node* node,
                             UmtsLayer3Data *umtsL3,
                             UmtsLayer3Ue* ueL3,
                             UmtsFlowSrcDestType srcDestType,
                             unsigned char transId)
{
    UmtsLayer3UeAppInfo* appInfo;
    char pktBuff[UMTS_MAX_PACKET_BUFFER_SIZE];
    UmtsLayer3ActivatePDPContextRequest *pdpReqMsg;
    int index = 0;
    unsigned char tfCls;
    unsigned char handlingPrio;

    if (DEBUG_SM)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_SM);
        printf("\n\t send an ACTIVATE PDP CONTEXT REQUEST for TransId %d\n",
               transId);
    }
    appInfo = UmtsLayer3UeFindAppInfoFromList(
        node, ueL3, srcDestType, transId);

    memset(pktBuff, 0, UMTS_MAX_PACKET_BUFFER_SIZE);
    pdpReqMsg = (UmtsLayer3ActivatePDPContextRequest *) &(pktBuff[0]);
    index += sizeof(UmtsLayer3ActivatePDPContextRequest);

    // fill the contents
    // set traffic class
    UmtsLayer3ConvertTrafficClassToOctet(
        appInfo->qosInfo->trafficClass,
        &tfCls,
        &handlingPrio);
    pdpReqMsg->trafficClass = tfCls;
    pdpReqMsg->trafficHandlingPrio = handlingPrio;
    pdpReqMsg->nsapi = appInfo->nsapi;

    // max bit rate
    UmtsLayer3ConvertMaxRateToOctet(appInfo->qosInfo->ulMaxBitRate,
                                    &pdpReqMsg->ulMaxBitRate);
    UmtsLayer3ConvertMaxRateToOctet(appInfo->qosInfo->dlMaxBitRate,
                                    &pdpReqMsg->dlMaxBitRate);
    UmtsLayer3ConvertMaxRateToOctet(appInfo->qosInfo->ulGuaranteedBitRate,
                                    &pdpReqMsg->ulGuaranteedBitRate);
    UmtsLayer3ConvertMaxRateToOctet(appInfo->qosInfo->dlGuaranteedBitRate,
                                    &pdpReqMsg->dlGuaranteedBitRate);

    // delay
    /*if (appInfo->qosInfo->trafficClass < UMTS_QOS_CLASS_INTERACTIVE_3)*/
    {
        // need set delay
        unsigned char delayVal;
        UmtsLayer3ConvertDelayToOctet(appInfo->qosInfo->transferDelay,
                                      &delayVal);

        pdpReqMsg->transferDelay = delayVal;

    }

    pdpReqMsg->qosIeLen = 13;

    // others
    pdpReqMsg->pdpOrgType = 1; // IETF
    pdpReqMsg->pdpAddrLen = 4; // IPv4, 16 Ipv6
    // appInfo->classifierInfo->srcAddr

    if (srcDestType == UMTS_FLOW_MOBILE_ORIGINATED)
    {
        NodeAddress ipv4Addr =
            GetIPv4Address(appInfo->classifierInfo->dstAddr);
        memcpy(&pktBuff[index], (char*)&ipv4Addr, pdpReqMsg->pdpAddrLen);
        index += pdpReqMsg->pdpAddrLen;
    }

    // add extend maxBitRate
    UmtsLayer3UeSendNasMsg(
        node,
        umtsL3,
        pktBuff,
        index,
        (char)UMTS_LAYER3_PD_SM,
        (char)UMTS_SM_MSG_TYPE_ACTIVATE_PDP_REQUEST,
        transId,
        UMTS_LAYER3_CN_DOMAIN_PS);
}

// /**
// FUNCTION   :: UmtsLayer3UeSmInitActivatePDPContextRequest
// LAYER      :: Layer 3
// PURPOSE    :: Send PDP activation request
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + nodeInput : const NodeInput* : Pointer to node input.
// + umtsL3    : UmtsLayer3Data*  : Pint to the umts L3 data
// + ueL3      : UmtsLayer3Ue *   : Pointer to UMTS UE Layer3 data
// + flowInfo  : UmtsLayer3UeAppInfo* : App Flow info
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeSmInitActivatePDPContextRequest(
                             Node* node,
                             UmtsLayer3Data *umtsL3,
                             UmtsLayer3Ue* ueL3,
                             UmtsLayer3UeAppInfo* flowInfo)
{
    UmtsLayer3UeSmSendActivatePDPContextRequest(
        node,
        umtsL3,
        ueL3,
        flowInfo->srcDestType,
        flowInfo->transId);
    ueL3->stat.numActivatePDPContextReqSent ++;

    flowInfo->cmData.smData->counterT3380 = 0;
    flowInfo->cmData.smData->needSendActivateReq = FALSE;
    flowInfo->cmData.smData->smState = UMTS_SM_MS_PDP_ACTIVE_PENDING;

    // start T3380
    if (flowInfo->cmData.smData->T3380 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, flowInfo->cmData.smData->T3380);
        flowInfo->cmData.smData->T3380 = NULL;
    }

    unsigned char info[2];
    info[0] = flowInfo->transId;
    info[1] = (unsigned char)flowInfo->srcDestType;
    flowInfo->cmData.smData->T3380 = UmtsLayer3SetTimer(
                                         node,
                                         umtsL3,
                                         UMTS_LAYER3_TIMER_T3380,
                                         UMTS_LAYER3_TIMER_T3380_VAL,
                                         NULL,
                                         (void*)&info[0],
                                         2);
}

//
// Build RRC Msg

// /**
// FUNCTION   :: UmtsUeRrcBuildRrcConnReq
// LAYER      :: Layer3 RRC
// PURPOSE    :: Build RRC_CONNECTION_REQUEST message
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + cause     : UmtsRrcEstCause: Establishment Cause
// + initUeId  : NodeId       : Initial UE identifier
// + protErrIndi : BOOL       : Protocol Error indicator
// + domainIndi : UmtsLayer3CnDomainId: Domain indicator
// RETURN     :: Message* : Point to the message containing DSC-REQ PDU
// **/
static
Message* UmtsLayer3UeBuildRrcConnReq(
             Node* node,
             UmtsRrcEstCause cause,
             UInt32 initUeId,
             BOOL protErrIndi,
             UmtsLayer3CnDomainId domainIndi)
{
    unsigned char buffer[8];
    int index = 0;
    buffer[index ++] = (unsigned char)(cause);
    memcpy(&buffer[index], &initUeId, sizeof(initUeId));
    index += sizeof(initUeId);
    buffer[index ++] = (unsigned char)protErrIndi;
    buffer[index ++] = (unsigned char)(domainIndi);

    Message* msg =
        MESSAGE_Alloc(node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node, msg, index, TRACE_UMTS_LAYER3);
    memcpy(MESSAGE_ReturnPacket(msg), buffer, index);

    UmtsLayer3AddHeader(
        node,
        msg,
        UMTS_LAYER3_PD_RR,
        0,
        (char)UMTS_RR_MSG_TYPE_RRC_CONNECTION_REQUEST);

    return msg;
}

// /**
// FUNCTION   :: UmtsLayer3UeBuildRrcConnSetupComp
// LAYER      :: Layer3 RRC
// PURPOSE    :: Build RRC_CONNECTION_SETUP_COMPLETE message
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + transctId : unsigned char         : transaction id
// RETURN     :: Message* : Point to the message containing DSC-REQ PDU
// **/
static
Message* UmtsLayer3UeBuildRrcConnSetupComp(
             Node* node,
             unsigned char transctId)
{
    int index = 0;
    Message* msg =
        MESSAGE_Alloc(node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node, msg, index, TRACE_UMTS_LAYER3);

    UmtsLayer3AddHeader(
        node,
        msg,
        UMTS_LAYER3_PD_RR,
        transctId,
        (unsigned char)UMTS_RR_MSG_TYPE_RRC_CONNECTION_SETUP_COMPLETE);

    return msg;
}

#if 0
// /**
// FUNCTION   :: UmtsLayer3UeBuildRrcConnRelComp
// LAYER      :: Layer3 RRC
// PURPOSE    :: Build RRC_CONNECTION_RELEASE_COMPLETE message
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + transctId : unsigned char         : transaction id
// RETURN     :: Message* : Point to the message containing DSC-REQ PDU
// **/
static
Message* UmtsLayer3UeBuildRrcConnRelComp(
             Node* node,
             unsigned char transctId)
{
    int index = 0;
    Message* msg =
        MESSAGE_Alloc(node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node, msg, index, TRACE_UMTS_LAYER3);

    UmtsLayer3AddHeader(
        node,
        msg,
        UMTS_LAYER3_PD_RR,
        transctId,
        (unsigned char)UMTS_RR_MSG_TYPE_RRC_CONNECTION_RELEASE_COMPLETE);

    return msg;
}
#endif

// FUNCTION   :: UmtsLayer3UeSendRrcConnReq
// LAYER      :: Layer3 RRC
// PURPOSE    :: Send a RRC connection requestion message
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + domainIndi : UmtsLayer3CnDomainId: Domain indicator
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeSendRrcConnReq(
    Node* node,
    UmtsLayer3Data *umtsL3,
    UmtsLayer3CnDomainId domainIndi)
{
    UmtsLayer3Ue *ueL3 = (UmtsLayer3Ue *) (umtsL3->dataPtr);
    UmtsRrcUeData* ueRrc = ueL3->rrcData;

    //If connection is already established
    if (ueRrc->state != UMTS_RRC_STATE_IDLE ||
        ueRrc->subState != UMTS_RRC_SUBSTATE_CAMPED_NORMALLY)
    {
        return;
    }
    //Or if a RRC connection request has been sent
    else if (ueRrc->timerT300 != NULL)
    {
        return;
    }
    //Or if ue is still waiting after rejected by UTRAN
    else if (ueRrc->timerRrcConnReqWaitTime != NULL)
    {
        return;
    }
    else if (ueRrc->counterV300 > UMTS_LAYER3_TIMER_N300)
    {
        //Enter into idle state
        ueRrc->counterV300 = 0;
        UmtsLayer3UeEnterIdle(node, umtsL3, ueL3);
        UmtsLayer3UeRrcConnEstInd(node, umtsL3, FALSE);

        // do cell reselection
        UmtsLayer3UeSearchNewCell(node, umtsL3, ueL3);
        return;
    }
    //Request RACH (RB0) setup and Access Sevice class mapping
    if (!ueRrc->rachOn)
    {
        UmtsLayer3UeConfigRandomAccessChannel(node,
                                              umtsL3,
                                              ueL3);
        ueRrc->rachOn = TRUE;
    }

    Message* msg = UmtsLayer3UeBuildRrcConnReq(
                        node,
                        ueRrc->estCause,
                        ueRrc->ueId,
                        ueRrc->protocolErrorIndi,
                        domainIndi);

    //Use RB0 to send the message,
    UmtsLayer3UeSendPacketToLayer2(node,
                                   umtsL3->interfaceIndex,
                                   UMTS_CCCH_RB_ID,
                                   msg);
    if (DEBUG_RR)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n\t sent a RRC_CONNECTION_REQUEST message "
            "with Domain: %d\n",domainIndi);
    }
    Message* timerMsg = UmtsLayer3SetTimer(node,
                                           umtsL3,
                                           UMTS_LAYER3_TIMER_T300,
                                           UMTS_LAYER3_TIMER_T300_VAL,
                                           NULL,
                                           &domainIndi,
                                           sizeof(UmtsLayer3CnDomainId));
    ueRrc->timerT300 = timerMsg;
    ueRrc->counterV300++;
}
// FUNCTION   :: UmtsLayer3UeSendRrcConnSetupComp
// LAYER      :: Layer3 RRC
// PURPOSE    :: Send RRC CONNECTION SETUP COMPLETE message to RNC
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// RETURN     :: NULL
// **/
static
void UmtsLayer3UeSendRrcConnSetupComp(
             Node* node,
             UmtsLayer3Data *umtsL3)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*) umtsL3->dataPtr;
    UmtsRrcUeData* ueRrc = ueL3->rrcData;

    unsigned char transctId = 0;
    Message* msg = UmtsLayer3UeBuildRrcConnSetupComp(node, transctId);

    UmtsLayer3UeSendPacketToLayer2(node,
                                   umtsL3->interfaceIndex,
                                   UMTS_SRB1_ID,
                                   msg);
    if (DEBUG_RR)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n\t sends a RRC_CONNECTION_SETUP_COMPLETE "
            "message to RNC\n");
    }

    ueRrc->counterV300 = 0;
}

// /**
// FUNCTION   :: UmtsLayer3UeSendRrcConnRelComp
// LAYER      :: Layer3 RRC
// PURPOSE    :: Send RRC CONNECTION RELEASE COMPLETE message to RNC
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// RETURN     :: NULL
// **/
static
void UmtsLayer3UeSendRrcConnRelComp(
             Node* node,
             UmtsLayer3Data *umtsL3)
{
    Message* replyMsg =
        MESSAGE_Alloc(node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        replyMsg,
                        sizeof(NodeId),
                        TRACE_UMTS_LAYER3);
    memcpy(MESSAGE_ReturnPacket(replyMsg),
           &(node->nodeId),
           sizeof(NodeId));

    UmtsLayer3UeSendRrcMsg(
        node,
        umtsL3,
        UMTS_RR_MSG_TYPE_RRC_CONNECTION_RELEASE_COMPLETE,
        replyMsg,
        UMTS_SRB1_ID);
}

// /**
// FUNCTION   :: UmtsLayer3UeSendRrcStatus
// LAYER      :: Layer3 RRC
// PURPOSE    :: Send a RRC STATUS message to RNC
// PARAMETERS ::
// + node       : Node*                 : Pointer to node.
// + umtsL3     : UmtsLayer3Data *      : Pointer to UMTS Layer3 data
// + rcvMsgType : UmtsRRMessageType     : The received message type
// + transctId  : unsinged char         : Transaction ID of received message
// + errCause   : UmtsRrcPtclErrCause   : Protocol error cause for
//                                        the received message
// RETURN     :: NULL
// **/
static
void UmtsLayer3UeSendRrcStatus(
    Node* node,
    UmtsLayer3Data* umtsL3,
    UmtsRRMessageType rcvMsgType,
    unsigned char transctId,
    UmtsRrcPtclErrCause errCause)
{
    //UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*) umtsL3->dataPtr;

    char buffer[3];
    int index = 0;
    buffer[index++] = (char)rcvMsgType;
    buffer[index++] = (char)transctId;
    buffer[index++] = (char)errCause;

    Message* msg =
        MESSAGE_Alloc(node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node, msg, index, TRACE_UMTS_LAYER3);
    memcpy(MESSAGE_ReturnPacket(msg), buffer, index);

    UmtsLayer3AddHeader(
        node,
        msg,
        UMTS_LAYER3_PD_RR,
        0,
        (char)UMTS_RR_MSG_TYPE_RRC_STATUS);
    UmtsLayer3UeSendPacketToLayer2(node,
                                   umtsL3->interfaceIndex,
                                   2,
                                   msg);
}

// /**
// FUNCTION   :: UmtsLayer3UeSendMeasurementReport
// LAYER      :: Layer3 RRC
// PURPOSE    :: Send a RRC STATUS message to RNC
// PARAMETERS ::
// + node       : Node*                 : Pointer to node.
// + umtsL3     : UmtsLayer3Data *      : Pointer to UMTS Layer3 data
// + ueL3       : UmtsLayer3Ue*         : Ue data
// RETURN     :: NULL
// **/
static
void UmtsLayer3UeSendMeasurementReport(
    Node* node,
    UmtsLayer3Data* umtsL3,
    UmtsLayer3Ue* ueL3)
{
    char pktBuff[UMTS_MAX_PACKET_BUFFER_SIZE];
    UmtsLayer3MeasurementReport* measReport;
    int index = 0;

    if (DEBUG_MEASUREMENT)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n\t send a measurement report to network\n");
    }

    memset(pktBuff, 0, UMTS_MAX_PACKET_BUFFER_SIZE);
    measReport = (UmtsLayer3MeasurementReport *) &(pktBuff[0]);
    index += sizeof(UmtsLayer3MeasurementReport);
    measReport->measId = 1; // canned right now

    std::list <UmtsLayer3UeNodebInfo*>::iterator it;
    clocktype curTime = node->getNodeTime();
    unsigned char numMeas = 0;


    for (it = ueL3->ueNodeInfo->begin();
        it != ueL3->ueNodeInfo->end();
        it ++)
    {
        signed char rscpVal;
        if (((*it)->cellStatus == UMTS_CELL_ACTIVE_SET ||
            (*it)->cellStatus == UMTS_CELL_MONITORED_SET) &&
            (*it)->cpichRscpMeas->lastTimeStamp
            + ueL3->para.shoEvalTime > curTime &&
            (*it)->cpichRscp != 0)
        {
            memcpy(&pktBuff[index],
                   &((*it)->cellId),
                   sizeof((*it)->cellId));
            index += sizeof((*it)->cellId); //  -90dbm ~ -25dbm
            rscpVal = (signed char) ((*it)->cpichRscp);
            if (DEBUG_MEASUREMENT )
            {
                printf("\t to cell %d rscp is %f %d\n",
                       (*it)->cellId, (*it)->cpichRscp, rscpVal);
            }
            if (rscpVal < -120)
            {
                rscpVal = -120;
            }
            else if (rscpVal >= -25)
            {
                rscpVal = -25;
            }
            memcpy(&pktBuff[index], &rscpVal, 1);
            index ++;
            numMeas ++;
        }
    }
    measReport->numMeas = numMeas;

    ueL3->lastReportTime = curTime;

    Message* repMsg =
        MESSAGE_Alloc(node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        repMsg,
                        index,
                        TRACE_UMTS_LAYER3);

    memcpy(MESSAGE_ReturnPacket(repMsg),
           pktBuff,
           index);

    UmtsLayer3AddHeader(
        node,
        repMsg,
        UMTS_LAYER3_PD_RR,
        0,
        (char)UMTS_RR_MSG_TYPE_MEASUREMENT_REPORT);
    UmtsLayer3UeSendPacketToLayer2(node,
                                   umtsL3->interfaceIndex,
                                   UMTS_SRB2_ID,
                                   repMsg);
}

// /**
// FUNCTION   :: UmtsLayer3UeSendActvSetUpComp
// LAYER      :: Layer3 RRC
// PURPOSE    :: Send a RRC STATUS message to RNC
// PARAMETERS ::
// + node       : Node*                 : Pointer to node.
// + umtsL3     : UmtsLayer3Data *      : Pointer to UMTS Layer3 data
// + ueL3       : UmtsLayer3Ue*         : Ue data
// + cellId     : UInt32                : cell Id of nodeB
// + addCell    : BOOL                  : add cell or not
// RETURN     :: NULL
// **/
static
void UmtsLayer3UeSendActvSetUpComp(
    Node* node,
    UmtsLayer3Data* umtsL3,
    UmtsLayer3Ue* ueL3,
    UInt32 cellId,
    BOOL   addCell)
{
    Message* msg =
        MESSAGE_Alloc(node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        msg,
                        sizeof(cellId) + sizeof(char),
                        TRACE_UMTS_LAYER3);
    char* packet = MESSAGE_ReturnPacket(msg);
    int index = 0;
    packet[index++] = (char)addCell;
    memcpy(&packet[index], &cellId, sizeof(cellId));
    index += sizeof(cellId);

    UmtsLayer3UeSendRrcMsg(
        node,
        umtsL3,
        UMTS_RR_MSG_TYPE_ACTIVE_SET_UPDATE_COMPLETE,
        msg);
}

// /**
// FUNCTION   :: UmtsLayer3UeSendSignalConnRelIndi
// LAYER      :: Layer3 RRC
// PURPOSE    :: Send a SIGNAL_CONNECTION_RELEASE_INDICATION
//               message to the RNC
// PARAMETERS ::
// + node       : Node*                 : Pointer to node.
// + umtsL3     : UmtsLayer3Data *      : Pointer to UMTS Layer3 data
// + ueL3       : UmtsLayer3Ue*         : Ue data
// + domain     : UmtsLayer3CnDomainId  : Domain Id
// RETURN     :: NULL
// **/
static
void UmtsLayer3UeSendSignalConnRelIndi(
    Node* node,
    UmtsLayer3Data* umtsL3,
    UmtsLayer3Ue* ueL3,
    UmtsLayer3CnDomainId domain)
{
    ueL3->rrcData->signalConn[domain] = FALSE;
    Message* msg =
        MESSAGE_Alloc(node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        msg,
                        sizeof(char),
                        TRACE_UMTS_LAYER3);
    char* packet = MESSAGE_ReturnPacket(msg);
    (*packet) = (char)domain;
    UmtsLayer3UeSendRrcMsg(
        node,
        umtsL3,
        UMTS_RR_MSG_TYPE_SIGNALLING_CONNECTION_RELEASE_INDICATION,
        msg);
}

//
// handle GMM Msg
//
//--------------------------------------------------------------------------
// FUNCTION interact with APP
//--------------------------------------------------------------------------
// /**
// FUNCTION   :: UmtsLayer3UeHandleStartCallMsg
// LAYER      :: Layer3
// PURPOSE    :: Handle Call start msg from APP.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data *  : Pointer to UMTS Layer3 data
// + msg              : Message*          : Message for node to interpret.
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeHandleStartCallMsg(Node *node,
                                    UmtsLayer3Data* umtsL3,
                                    Message *msg)
{
    // get the umts call src dest info
    // initiate call setup
    // start timers
    // timer expires or call setup finished
    // send APP call accpet/call reject msg

    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*) (umtsL3->dataPtr);
    AppUmtsCallStartMessageInfo* callInfo = (AppUmtsCallStartMessageInfo*)
                                             MESSAGE_ReturnInfo(msg);

    ERROR_Assert(UmtsLayer3UeFindUmtsCallApp(
                    node, ueL3, callInfo->appSrcNodeId, callInfo->appId)
                 == NULL,
                 "Umts call application already exists.");
    ueL3->stat.numMoAppRcvdFromUpper ++;
    ueL3->stat.numMoCsAppRcvdFromUpper ++;

    int pos = UmtsGetPosForNextZeroBit<UMTS_MAX_TRANS_MOBILE>
                                      (*(ueL3->transIdBitSet), 0);

    if (pos == -1)
    {
        UmtsLayer3UeNotifyAppCallReject(
            node,
            umtsL3,
            callInfo,
            APP_UMTS_CALL_REJECT_CAUSE_TOO_MANY_ACTIVE_APP);
        ueL3->stat.numMoAppRejected ++;
        ueL3->stat.numMoCsAppRejected ++;
        return;
    }

    char transId = (unsigned char)pos;
    UmtsLayer3UeAppInfo* ueFlow = UmtsLayer3UeAddAppInfoToList(
                                           node,
                                           ueL3,
                                           UMTS_FLOW_MOBILE_ORIGINATED,
                                           UMTS_LAYER3_CN_DOMAIN_CS,
                                           transId);
    if (!ueFlow)
    {
        MESSAGE_Free(node, msg);

        ERROR_ReportWarning("fail to create appInfo, Call dropped");

        return;
    }
    else
    {
        ueL3->transIdBitSet->set(transId);
    }

    UmtsCcMsData* ueCc = ueFlow->cmData.ccData;
    ueCc->peerAddr = callInfo->appDestNodeId;
    ueCc->callAppId = callInfo->appId;
    ueCc->endTime = callInfo->callEndTime;
    ueCc->avgTalkTime = callInfo->avgTalkTime;

    UmtsLayer3UeInitMsCallSetup(node,
                                umtsL3,
                                ueL3,
                                ueFlow);
    ueL3->stat.numMoAppInited ++;
    ueL3->stat.numMoCsAppInited ++;
}

// /**
// FUNCTION   :: UmtsLayer3UeHandleEndCallMsg
// LAYER      :: Layer3
// PURPOSE    :: Handle Call Endt msg from APP.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data *  : Pointer to UMTS Layer3 data
// + msg              : Message*          : Message for node to interpret.
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeHandleEndCallMsg(Node *node,
                                  UmtsLayer3Data* umtsL3,
                                  Message *msg)
{
    // get the umts call src dest info
    // initiate call disconnect
    // start timers
    // timers expires or call disconnect finished
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*)(umtsL3->dataPtr);
    AppUmtsCallEndMessageInfo* callInfo =
        (AppUmtsCallEndMessageInfo*)MESSAGE_ReturnInfo(msg);

    UmtsLayer3UeAppInfo* ueFlow = UmtsLayer3UeFindUmtsCallApp(
                                    node,
                                    ueL3,
                                    callInfo->appSrcNodeId,
                                    callInfo->appId);
    if (ueFlow)
    {
        UmtsLayer3UeInitCallClearing(node,
                                     umtsL3,
                                     ueFlow,
                                     UMTS_CALL_CLEAR_END);
    }
}

// /**
// FUNCTION   :: UmtsLayer3UeHandleCallAnsweredMsg
// LAYER      :: Layer3
// PURPOSE    :: Handle Call answered msg from APP.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data *  : Pointer to UMTS Layer3 data
// + msg              : Message*          : Message for node to interpret.
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeHandleCallAnsweredMsg(Node *node,
                                       UmtsLayer3Data* umtsL3,
                                       Message *msg)
{
    //Call was answered by the User, entering connect request state by
    //sending CONNECT message to the network
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*)(umtsL3->dataPtr);

    AppUmtsCallAnsweredMessageInfo* callInfo =
        (AppUmtsCallAnsweredMessageInfo*)MESSAGE_ReturnInfo(msg);

    UmtsLayer3UeAppInfo* ueFlow =
        UmtsLayer3UeFindAppInfoFromList(
            node,
            ueL3,
            UMTS_FLOW_NETWORK_ORIGINATED,
            (unsigned char)callInfo->transactionId);
    if (ueFlow && (ueFlow->cmData.ccData->state
                    == UMTS_CC_MS_MOBILE_TERMINATING_CALL_CONFIRMED
                   || ueFlow->cmData.ccData->state
                    == UMTS_CC_MS_CALL_RECEIVED))
    {
        UmtsLayer3UeSendMobTermCallConnect(
            node,
            umtsL3,
            ueFlow);
        ueFlow->cmData.ccData->T313
            = UmtsLayer3SetTimer(
                            node,
                            umtsL3,
                            UMTS_LAYER3_TIMER_CC_T313,
                            UMTS_LAYER3_TIMER_CC_T313_MS_VAL,
                            NULL,
                            (void*)&(ueFlow->transId),
                            sizeof(ueFlow->transId));
    }
    else
    {
        // do nothing
    }
}

// /**
// FUNCTION   :: UmtsLayer3UeHandleCallPktArrival
// LAYER      :: Layer3
// PURPOSE    :: Handle Call answered msg from APP.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data *  : Pointer to UMTS Layer3 data
// + msg              : Message*          : Message for node to interpret.
// RETURN     :: void : NULL
// **/
static
BOOL UmtsLayer3UeHandleCallPktArrival(Node *node,
                                      UmtsLayer3Data* umtsL3,
                                      Message *msg)
{
    BOOL msgReused = FALSE;
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*)(umtsL3->dataPtr);

    ueL3->stat.numCsDataPacketFromUpper ++;
    AppUmtsCallPktArrivalInfo* callInfo =
        (AppUmtsCallPktArrivalInfo*) MESSAGE_ReturnInfo(msg);
    UmtsLayer3UeAppInfo* ueFlow = UmtsLayer3UeFindUmtsCallApp(
                                        node,
                                        ueL3,
                                        callInfo->appSrcNodeId,
                                        callInfo->appId);
    if (ueFlow)
    {
        UmtsCcMsData* ueCc = (UmtsCcMsData*)(ueFlow->cmData.ccData);
        if (ueCc->state == UMTS_CC_MS_ACTIVE)
        {
            ERROR_Assert(ueFlow->rabId != UMTS_INVALID_RAB_ID &&
                ueFlow->rbId != UMTS_INVALID_RB_ID,
                "Invalid CC state!!!");
            UmtsLayer3UeSendPacketToLayer2(
                node,
                umtsL3->interfaceIndex,
                ueFlow->rbId,
                msg);
            ueL3->stat.numCsDataPacketSentToL2 ++;
            msgReused = TRUE;
        }
        else
        {
            // Report to UmtsCallApp a protocol error?
            ueL3->stat.numCsPktDroppedNoResource ++;
        }
    }
    else
    {
        ueL3->stat.numCsPktDroppedNoResource ++;
    }
    return msgReused;
}

//--------------------------------------------------------------------------
// Handle msg/cmd FUNCTIONs
//--------------------------------------------------------------------------
// handle RR Msg

#if 0
// /**
// FUNCTION   :: UmtsLayer3UeHandleTransctId
// LAYER      :: Layer3 RRC
// PURPOSE    :: Handle Transaction ID presented in certain messages
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + transctId : unsinged char         : transaction id
// + msgType   : UmtsRRMessageType*     : RRC message type.
// RETURN     :: BOOL         : Whether to discard the message
// **/
static
BOOL UmtsLayer3UeHandleTransctId(
        Node* node,
        UmtsLayer3Data *umtsL3,
        UmtsRRMessageType msgType,
        unsigned char transctId)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*) umtsL3->dataPtr;
    UmtsRrcUeData* ueRrc = ueL3->rrcData;

    std::map<UmtsRRMessageType, unsigned char>::iterator pos;
    BOOL discardMsg = FALSE;
    switch (msgType)
    {
        case UMTS_RR_MSG_TYPE_RADIO_BEARER_SETUP:
        case UMTS_RR_MSG_TYPE_RADIO_BEARER_RECONFIGURATION:
        case UMTS_RR_MSG_TYPE_RADIO_BEARER_RELEASE:
        case UMTS_RR_MSG_TYPE_TRANSPORT_CHANNEL_RECONFIGURATION:
        case UMTS_RR_MSG_TYPE_PHYSICAL_CHANNEL_RECONFIGURATION:
        {
            if (ueRrc->orderedReconfiguration == FALSE &&
                ueRrc->cellUpStarted == FALSE &&
                ueRrc->acceptedTranst->find(msgType) ==
                        ueRrc->acceptedTranst->end())
            {
                (*ueRrc->acceptedTranst)[msgType] = transctId;
            }
            else
            {
                discardMsg = TRUE;
            }
            break;
        }
        case UMTS_RR_MSG_TYPE_RRC_CONNECTION_SETUP:
        case UMTS_RR_MSG_TYPE_CELL_UPDATE_CONFIRM:
        case UMTS_RR_MSG_TYPE_URA_UPDATA_CONFIRM:
        case UMTS_RR_MSG_TYPE_UE_CAPABILITY_ENQUIRY:
        {
            pos = ueRrc->acceptedTranst->find(msgType);
            if (pos == ueRrc->acceptedTranst->end())
            {
                (*ueRrc->acceptedTranst)[msgType] = transctId;
            }
            else if (pos->second != transctId)
            {
                pos->second = transctId;
            }
            else
            {
                discardMsg = TRUE;
            }
            break;
        }
        default:
        {

        }
    }
    return discardMsg;
}

// /**
// FUNCTION   :: UmtsLayer3UeEraseTransct
// LAYER      :: Layer3 RRC
// PURPOSE    :: Erase Transaction associate with a message type
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + msgType   : UmtsRRMessageType*     : RRC message type.
// RETURN     :: BOOL         : Whether this transaction exists
// **/
static
BOOL UmtsLayer3UeEraseTransct(
        Node* node,
        UmtsLayer3Data *umtsL3,
        UmtsRRMessageType msgType)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*) umtsL3->dataPtr;
    UmtsRrcUeData* ueRrc = ueL3->rrcData;

    BOOL retVal = FALSE;
    std::map<UmtsRRMessageType, unsigned char>::iterator pos =
                            ueRrc->acceptedTranst->find(msgType);
    if (pos != ueRrc->acceptedTranst->end())
    {
        ueRrc->acceptedTranst->erase(pos);
        retVal = TRUE;
    }
}
#endif

// /**
// FUNCTION   :: UmtsLayer3UeHandleRrcConnSetup
// LAYER      :: Layer3 RRC
// PURPOSE    :: Handle RRC_CONNECTION_SETUP message
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + transctId : char         : transaction id
// + msg       : Message*     : RRC message.
// RETURN     :: NULL
// **/
static
void UmtsLayer3UeHandleRrcConnSetup(
             Node* node,
             UmtsLayer3Data *umtsL3,
             unsigned char transctId,
             Message* msg)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*) umtsL3->dataPtr;
    UmtsRrcUeData* ueRrc = ueL3->rrcData;
    char* msgContent = MESSAGE_ReturnPacket(msg);

    int index = 0;
    NodeId initUeId;
    memcpy(&initUeId, msgContent, sizeof(initUeId));
    if (ueRrc->ueId != initUeId)
    {
        return;
    }
    if (DEBUG_RR )
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n\treceived a RRC_CONNECTION_SETUP message\n ");
    }

    index += sizeof(initUeId);
    char stateIndi;
    stateIndi = msgContent[index++];
    UmtsRrcConnSetupSpecMode specMode = (UmtsRrcConnSetupSpecMode)
                                        msgContent[index++];
    UInt32 ueScCode = 0;
    UmtsRcvPhChInfo dlDpdchInfo;

    // update stats
    ueL3->stat.numRrcConnSetupRcvd ++;

    if (specMode == UMTS_RRCCONNSETUP_SPECMODE_PRECONFIG)
    {
        UmtsRrcConnSetupPreConfigMode preconfigMode =
            (UmtsRrcConnSetupPreConfigMode) msgContent[index++];
        if (preconfigMode == UMTS_RRCCONNSETUP_PRECONFIGMODE_DEFAULT)
        {
            if (initUeId != ueRrc->ueId || ueRrc->timerT300 == NULL)
            {
                return;
            }
            memcpy(&ueScCode, &msgContent[index], sizeof(ueScCode));
            index += sizeof(ueScCode);
            memcpy(&dlDpdchInfo,
                   &msgContent[index],
                   sizeof(UmtsRcvPhChInfo));
            index += sizeof(UmtsRcvPhChInfo);
        }
        else if (preconfigMode == UMTS_RRCCONNSETUP_PRECONFIGMODE_PREDEFINE)
        {
            ERROR_ReportError("UmtsLayer3UeHandleRrcConnSetup: "
                "predefine preconfiguration mode isn't allowed. ");

        }
    }
    else
    {
        ERROR_ReportError("UmtsLayer3UeHandleRrcConnSetup: "
            "Complete specification mode isn't allowed. ");
    }

    if (DEBUG_RR)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n\treceived a RRC_CONNECTION_SETUP message "
            "with TI: %d and UE primary scrambling code: %u \n"
            "The downlink DPDCH info: "
            "Spreading Factor: %d, OVSF channel code: %u \n",
            transctId, ueScCode,
            dlDpdchInfo.sfFactor, dlDpdchInfo.chCode);
    }

    UmtsLayer3UeConfigSignalRb1To3(node,
                                   umtsL3,
                                   ueScCode,
                                   &dlDpdchInfo);

    ueRrc->estCause = UMTS_RRC_ESTCAUSE_CLEARED;
    ERROR_Assert(ueRrc->timerT300 != NULL,
        "T300 should be started when "
        "RRC_CONNECTION_REQUEST has been sent!");
    MESSAGE_CancelSelfMsg(node, ueRrc->timerT300);
    ueRrc->timerT300 = NULL;

    ueRrc->state = UMTS_RRC_STATE_CONNECTED;
    ueRrc->subState = UMTS_RRC_SUBSTATE_CELL_DCH;
    std::list<UmtsLayer3UeNodebInfo*>::iterator nodebIter;
    for (nodebIter = ueL3->ueNodeInfo->begin();
         nodebIter != ueL3->ueNodeInfo->end();
         nodebIter++)
    {
        if ((*nodebIter)->cellId == ueRrc->selectedCellId)
        {
            (*nodebIter)->cellStatus = UMTS_CELL_ACTIVE_SET;

            // GUI
            if (node->guiOption)
            {
                GUI_SendIntegerData(node->nodeId,
                                    ueL3->stat.activeSetSizeGuiId,
                                    UmtsLayer3UeGetCellStatusSetSize(
                                       node, umtsL3, ueL3,
                                       UMTS_CELL_ACTIVE_SET),
                                    node->getNodeTime());
            }

            break;
        }
    }

    UmtsLayer3UeSendRrcConnSetupComp(node, umtsL3);
    // update stats
    ueL3->stat.numRrcConnSetupCompSent ++;

    UmtsLayer3UeRrcConnEstInd(node, umtsL3, TRUE);
}

// /**
// FUNCTION   :: UmtsLayer3UeHandleRrcConnRej
// LAYER      :: Layer3 RRC
// PURPOSE    :: Handle RRC_CONNECTION_REJECT message
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + transctId : unsigned char         : transaction id
// + msg       : Message*     : RRC message.
// RETURN     :: NULL
// **/
static
void UmtsLayer3UeHandleRrcConnRej(
             Node* node,
             UmtsLayer3Data *umtsL3,
             unsigned char transctId,
             Message* msg)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*) umtsL3->dataPtr;
    UmtsRrcUeData* ueRrc = ueL3->rrcData;
    char* msgContent = MESSAGE_ReturnPacket(msg);
    int index = 0;
    UInt32 initUeId;
    memcpy(&initUeId, msgContent + index, sizeof(initUeId));
    index += sizeof(initUeId);
    clocktype waitTime;
    memcpy(&waitTime, &msgContent[index++], sizeof(waitTime));
    index += sizeof(waitTime);

    if (DEBUG_RR)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n\tReceived a RRC_CONNECTION_REJECT message "
            "with TI: %d \n",
            (unsigned int)transctId);
    }

    // update stats
    ueL3->stat.numRrcConnSetupRejRcvd ++;

    if (initUeId != ueRrc->ueId
        || ueRrc->state == UMTS_RRC_STATE_IDLE
        || ueRrc->timerT300 == NULL)
    {
        return;
    }

    MESSAGE_CancelSelfMsg(node, ueRrc->timerT300);
    ueRrc->timerT300 = NULL;

    Message* timerMsg = UmtsLayer3SetTimer(
                            node,
                            umtsL3,
                            UMTS_LAYER3_TIMER_RRCCONNWAITTIME,
                            UMTS_LAYER3_TIMER_RRCCONNWAITTIME_VAL,
                            NULL,
                            NULL,
                            0);
    ueRrc->timerRrcConnReqWaitTime = timerMsg;
}

// /**
// FUNCTION   :: UmtsLayer3UeHandleRrcConnRel
// LAYER      :: Layer3 RRC
// PURPOSE    :: Handle RRC_CONNECTION_RELEASE message
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + transctId : unsigned char         : transaction id
// + msg       : Message*     : RRC message.
// RETURN     :: Message* : Point to the message containing DSC-REQ PDU
// **/
static
void UmtsLayer3UeHandleRrcConnRel(
             Node* node,
             UmtsLayer3Data *umtsL3,
             unsigned char transctId,
             Message* msg)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*) umtsL3->dataPtr;
    UmtsRrcUeData* ueRrc = ueL3->rrcData;
    if (DEBUG_RR_RELEASE)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n\tReceived a RRC_CONNECTION_RELEASE message\n");
    }

    char* msgContent = MESSAGE_ReturnPacket(msg);
    int index = 0;
    UInt32 ueId;
    memcpy(&ueId, msgContent + index, sizeof(ueId));
    index += sizeof(ueId);

    // update stats
    ueL3->stat.numRrcConnRelRcvd ++;

    if (ueId != node->nodeId)
    {
        return;
    }

    UmtsLayer3UeSendRrcConnRelComp(node, umtsL3);
    // update stat
    ueL3->stat.numRrcConnRelCompSent ++;

    // reset the rrc state to IDLE, but keep fire T308
    ueL3->rrcData->state = UMTS_RRC_STATE_IDLE;

    // set substate to CAMP_NORMALY
    ueL3->rrcData->subState =
        UMTS_RRC_SUBSTATE_CAMPED_NORMALLY;

    //Remove all signalling connections when leaving RRC connected state
    for (int i = 0; i < UMTS_MAX_CN_DOMAIN; i++)
    {
        ueL3->rrcData->signalConn[i]  = FALSE;
    }

    if (ueL3->rrcData->rachOn)
    {
        UmtsLayer3UeReleaseRandomAccessChannel(
            node,
            umtsL3,
            ueL3);
        ueL3->rrcData->rachOn = FALSE;
    }

    ERROR_Assert(ueRrc->timerT308 == NULL,
        "T308 should not be started!");
    ueRrc->timerT308 =
        UmtsLayer3SetTimer(node,
                           umtsL3,
                           UMTS_LAYER3_TIMER_T308,
                           UMTS_LAYER3_TIMER_T308_VAL,
                           NULL,
                           NULL,
                           0);
    ueRrc->counterV308 = 0;
}

// /**
// FUNCTION   :: UmtsLayer3UeHandleDlDirectTransfer
// LAYER      :: Layer3 RRC
// PURPOSE    :: Handle DOWNLINK_DIRECT_TRANSFER message
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + transctId : unsigned char         : transaction id
// + msg       : Message*     : RRC message.
// RETURN     :: NULL
// **/
static
void UmtsLayer3UeHandleDlDirectTransfer(
        Node* node,
        UmtsLayer3Data *umtsL3,
        unsigned char transctId,
        Message* msg)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*) umtsL3->dataPtr;
    UmtsRrcUeData* ueRrc = ueL3->rrcData;
    char* packet = MESSAGE_ReturnPacket(msg);

    UmtsLayer3CnDomainId domain = (UmtsLayer3CnDomainId)(*packet);

    if (DEBUG_RR)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n\tReceived a DOWNLINK DIRECT TRANSER "
            "message from RNC, with domain: %d. \n", domain);
    }

    // update stat
    ueL3->stat.numDlDirectTransferRcvd ++;

    if (ueRrc->signalConn[domain])
    {
        UmtsLayer3UeHandleNasMsg(node,
                                 umtsL3,
                                 packet+1,
                                 MESSAGE_ReturnPacketSize(msg)-1,
                                 domain);
    }
    else
    {
        UmtsRRMessageType rcvMsgType =
            UMTS_RR_MSG_TYPE_DL_DIRECT_TRANSFER;
        UmtsRrcPtclErrCause errCause =
            UMTS_RRC_PTCLERRCAUSE_MSGCONFICTSTATE;
        UmtsLayer3UeSendRrcStatus(node,
                                  umtsL3,
                                  rcvMsgType,
                                  transctId,
                                  errCause);
    }
}

// /**
// FUNCTION   :: UmtsLayer3UeHandlePaging
// LAYER      :: Layer3 RRC
// PURPOSE    :: Handle PAGING_TYPE_1/2 message
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + msg       : Message*     : RRC message.
// RETURN     :: NULL
// **/
static
void UmtsLayer3UeHandlePaging(
        Node* node,
        UmtsLayer3Data *umtsL3,
        Message* msg)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*) umtsL3->dataPtr;
    UmtsRrcUeData* ueRrc = ueL3->rrcData;

    NodeId ueId;
    UmtsLayer3CnDomainId domain;
    UmtsPagingCause cause;
    char* packet = MESSAGE_ReturnPacket(msg);
    memcpy(&ueId, packet, sizeof(ueId));
    packet += sizeof(ueId);
    domain = (UmtsLayer3CnDomainId)(*packet++);
    cause = (UmtsPagingCause)(*packet);

    if (ueId != ueRrc->ueId)
    {
        return;
    }

    if (DEBUG_RR)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n\tReceives a Paging message with cause: "
                "%d, and domain ID: %d.\n",
               cause, domain);
    }

    // update stats
    ueL3->stat.numPagingRcvd ++;

    if (domain == UMTS_LAYER3_CN_DOMAIN_PS)
    {
        unsigned char serviceType = (unsigned char)
                                    UMTS_GMM_SERVICE_TYPE_PAGINGRESP;

        UmtsLayer3UeInitServiceRequest(
            node,
            umtsL3,
            ueL3,
            UMTS_RRC_ESTCAUSE_TERMINATING_BACKGROUND,
            serviceType,
            0);
    }
    else if (domain == UMTS_LAYER3_CN_DOMAIN_CS)
    {
        //UmtsMmMsData* ueMm = ueL3->mmData;
        if (ueL3->mmData->mainState == UMTS_MM_MS_MM_IDLE)
        {
            ueL3->mmData->mainState =
                UMTS_MM_MS_WAIT_FOR_RR_ACTIVE;
        }
        UmtsLayer3UeInitPagingResp(
            node,
            umtsL3,
            ueL3);
    }
}
// /**
// FUNCTION   :: UmtsLayer3UeHandleHsdpaRbSetup
// LAYER      :: Layer3 RRC
// PURPOSE    :: Handle RADIO_BEARER_SETUP message for regular RB
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + transctId : char         : transaction id
// + msg       : Message*     : RRC message.
// RETURN     :: NULL
// **/
static
void UmtsLayer3UeHandleHsdpaRbSetup(
        Node* node,
        UmtsLayer3Data *umtsL3,
        char transctId,
        Message* msg)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*) umtsL3->dataPtr;
    UmtsRrcUeData* ueRrc = ueL3->rrcData;
    char* packet = MESSAGE_ReturnPacket(msg);
    int index = 0;

    if (DEBUG_RR || DEBUG_RB_HSDPA)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        std::cout << "UE: " << node->nodeId
                  << " received an RADIO_BEARER_SETUP"
                  << " message from RNC for a HSDPA RB" << std::endl;
    }

    UmtsUeRbSetupPara rbSetupPara;
    memcpy(&rbSetupPara, packet, sizeof(rbSetupPara));
    index += sizeof(UmtsUeRbSetupPara);

    if (DEBUG_RR || DEBUG_RB_HSDPA)
    {
        printf("\nConfigure current SfInUse to %d numUlDPdch %d\n",
            rbSetupPara.sfFactor, rbSetupPara.numUlDPdch);
    }

    int numCells = (int)packet[index++];
    for (int j = 0; j < numCells; j++)
    {
        UmtsRrcNodebResInfo cellInfo;
        memcpy(&cellInfo, &packet[index], sizeof(cellInfo));
        index += sizeof(cellInfo);
        int numPhCh = cellInfo.numDlDpdch;
        if (DEBUG_RR || DEBUG_RB_HSDPA)
        {
            std::cout << "Handling IE DL_DPCH_INFO: "
                      << "CELL ID: " << cellInfo.cellId
                      << " Number of channels: "
                      << numPhCh << std::endl;
        }
        for (int i = 0; i < numPhCh; i++)
        {
            if (DEBUG_RR || DEBUG_RB_HSDPA)
            {
                std::cout << "   Spreading Factor: "
                          << cellInfo.sfFactor[i]
                          << "   Channelization code: "
                          << cellInfo.chCode[i]
                          << std::endl;
            }

            UmtsRcvPhChInfo dlDpdchInfo;
            dlDpdchInfo.chCode = cellInfo.chCode[i];
            dlDpdchInfo.sfFactor = cellInfo.sfFactor[i];
            UmtsMacPhyConfigRcvPhCh(
                node,
                umtsL3->interfaceIndex,
                cellInfo.cellId,
                &dlDpdchInfo);
        }
    }

    int ulPhChId = rbSetupPara.ulDpdchId;
    if (rbSetupPara.rlSetupNeeded)
    {
        // config a UL HS-DPCCH, only one is needed
        UmtsCphyRadioLinkSetupReqInfo rlInfo;
        UmtsCphyChDescUlHsdpcch phChDesc;

        phChDesc.sfFactor = UMTS_SF_256;
        phChDesc.chCode = 1; // TODO: need further
        phChDesc.ulScCode = ueRrc->ueScCode;

        rlInfo.phChDir = UMTS_CH_UPLINK;
        rlInfo.ueId = node->nodeId;
        rlInfo.phChType = UMTS_PHYSICAL_HSDPCCH;
        rlInfo.phChId = 1; //ulPhChId;
        rlInfo.priSc = ueRrc->ueScCode;
        rlInfo.phChDesc = &phChDesc;
        UmtsLayer3ConfigRadioLink(node, umtsL3, &rlInfo);
    }

    //////////////////
    int ulTrChId = rbSetupPara.ulDchId;
    if (DEBUG_RR)
    {
        std::cout << "IE UL_TrCh added: " << ulTrChId
                  << std::endl;
    }
    UmtsCphyTrChConfigReqInfo trCfgInfo;
    trCfgInfo.trChDir = UMTS_CH_UPLINK;
    trCfgInfo.trChId = (unsigned char)ulTrChId;
    trCfgInfo.trChType = UMTS_TRANSPORT_HSDSCH;
    trCfgInfo.ueId = node->nodeId;
    trCfgInfo.logChPrio = rbSetupPara.logPriority;
    memcpy(&(trCfgInfo.transFormat),
        &(rbSetupPara.transFormat),
        sizeof(UmtsTransportFormat));

    UmtsLayer3ConfigTrCh(node, umtsL3, &trCfgInfo);

    char rabId = rbSetupPara.rabId;
    UmtsLayer3CnDomainId domain = rbSetupPara.domain;
    char rbId = rbSetupPara.rbId;
    if (DEBUG_RR)
    {
        std::cout << "IE RAB Info to Setup: "
                  << "RAB ID: " << (int)rabId
                  << "  RB ID: " << (int)rbId
                  << std::endl;
    }
    UmtsUeRabInfo* rabInfo = (UmtsUeRabInfo*)
                              MEM_malloc(sizeof(UmtsUeRabInfo));
    rabInfo->cnDomainId = domain;
    for (int i = 1; i < UMTS_MAX_RB_PER_RAB; i++)
    {
        rabInfo->rbIds[i] = UMTS_INVALID_RB_ID;
    }
    rabInfo->rbIds[0] = rbId;
    ueRrc->rabInfos[(int)rabId] = rabInfo;
    ueRrc->rbRabId[rbId - UMTS_RBINRAB_START_ID] = rabId;

    char logPriority = rbSetupPara.logPriority;
    UmtsCmacConfigReqRbInfo rbInfo;

    // dl RB
    rbInfo.chDir = UMTS_CH_DOWNLINK;
    rbInfo.logChId = rbSetupPara.ulDtchId;
    rbInfo.logChType = UMTS_LOGICAL_DTCH;
    rbInfo.logChPrio = logPriority;
    rbInfo.rbId = rbId;
    rbInfo.trChType = trCfgInfo.trChType;
    rbInfo.ueId = node->nodeId;
    rbInfo.trChId = trCfgInfo.trChId;
    rbInfo.releaseRb = FALSE;
    UmtsLayer3ConfigRadioBearer(
         node,
         umtsL3,
         &rbInfo,
         NULL,
         NULL,
         0);

    // Configure upLink rb
    // uplink RB for ackowledge right now maps to DPDCH as well
    // rather than HS-DPCCH
    UmtsPhysicalChannelType ulPhChType = UMTS_PHYSICAL_DPDCH;

    rbInfo.chDir = UMTS_CH_UPLINK;
    UmtsLayer3ConfigRadioBearer(
         node,
         umtsL3,
         &rbInfo,
         &ulPhChType,
         &ulPhChId,
         1,
         &(rbSetupPara.transFormat));

    // RLC
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
            rbInfo.ueId);

        UmtsLayer3ConfigRlcEntity(
            node,
            umtsL3,
            rbId,
            UMTS_RLC_ENTITY_RX,
            UMTS_RLC_ENTITY_TM,
            &rbSetupPara.rlcConfig.tmConfig,
            rbInfo.ueId);
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
            rbInfo.ueId);
        UmtsLayer3ConfigRlcEntity(
            node,
            umtsL3,
            rbId,
            UMTS_RLC_ENTITY_RX,
            UMTS_RLC_ENTITY_UM,
            &rbSetupPara.rlcConfig.umConfig,
            rbInfo.ueId);
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
            rbInfo.ueId);
    }
    else
    {
        ERROR_ReportError("UmtsLayer3NodebHandleNbapRbSetup: "
            "receives wrong RLC entity type to setup ");
    }

    std::string msgBuf;
    msgBuf += rbId;
    msgBuf += rabId;
    Message* repMsg =
        MESSAGE_Alloc(node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        repMsg,
                        (int)msgBuf.size(),
                        TRACE_UMTS_LAYER3);
    memcpy(MESSAGE_ReturnPacket(repMsg),
           msgBuf.data(),
           msgBuf.size());
    UmtsLayer3AddHeader(
        node,
        repMsg,
        UMTS_LAYER3_PD_RR,
        0,
        (char)UMTS_RR_MSG_TYPE_RADIO_BEARER_SETUP_COMPLETE);
    UmtsLayer3UeSendPacketToLayer2(node,
                                   umtsL3->interfaceIndex,
                                   UMTS_SRB2_ID,
                                   repMsg);
}

// /**
// FUNCTION   :: UmtsLayer3UeHandleRegularRbSetup
// LAYER      :: Layer3 RRC
// PURPOSE    :: Handle RADIO_BEARER_SETUP message for regular RB
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + transctId : char         : transaction id
// + msg       : Message*     : RRC message.
// RETURN     :: NULL
// **/
static
void UmtsLayer3UeHandleRegularRbSetup(
        Node* node,
        UmtsLayer3Data *umtsL3,
        char transctId,
        Message* msg)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*) umtsL3->dataPtr;
    UmtsRrcUeData* ueRrc = ueL3->rrcData;
    char* packet = MESSAGE_ReturnPacket(msg);
    int index = 0;

    if (DEBUG_RR)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        std::cout << "UE: " << node->nodeId
                  << " received an RADIO_BEARER_SETUP"
                  << " message from RNC for a regular RB" << std::endl;
    }

    UmtsUeRbSetupPara rbSetupPara;
    memcpy(&rbSetupPara, packet, sizeof(rbSetupPara));
    index += sizeof(UmtsUeRbSetupPara);

    // config the PHY chs spread factor
    UmtsUeSetSpreadFactorInUse(
        node, umtsL3->interfaceIndex, rbSetupPara.sfFactor);

    UmtsMacPhyUeSetDpdchNumber(
        node, umtsL3->interfaceIndex, rbSetupPara.numUlDPdch);
    if (DEBUG_RR)
    {
        printf("\nConfigure current SfInUse to %d numUlDPdch %d\n",
            rbSetupPara.sfFactor, rbSetupPara.numUlDPdch);
    }

    int numCells = (int)packet[index++];
    for (int j = 0; j < numCells; j++)
    {
        UmtsRrcNodebResInfo cellInfo;
        memcpy(&cellInfo, &packet[index], sizeof(cellInfo));
        index += sizeof(cellInfo);
        int numPhCh = cellInfo.numDlDpdch;
        if (DEBUG_RR)
        {
            std::cout << "Handling IE DL_DPCH_INFO: "
                      << "CELL ID: " << cellInfo.cellId
                      << " Number of channels: "
                      << numPhCh << std::endl;
        }
        for (int i = 0; i < numPhCh; i++)
        {
            if (DEBUG_RR)
            {
                std::cout << "   Spreading Factor: "
                          << cellInfo.sfFactor[i]
                          << "   Channelization code: "
                          << cellInfo.chCode[i]
                          << std::endl;
            }

            UmtsRcvPhChInfo dlDpdchInfo;
            dlDpdchInfo.chCode = cellInfo.chCode[i];
            dlDpdchInfo.sfFactor = cellInfo.sfFactor[i];
            UmtsMacPhyConfigRcvPhCh(
                node,
                umtsL3->interfaceIndex,
                cellInfo.cellId,
                &dlDpdchInfo);
        }
    }

    int ulPhChId = rbSetupPara.ulDpdchId;
    int ulTrChId = rbSetupPara.ulDchId;
    if (DEBUG_RR)
    {
        std::cout << "IE UL_TrCh added: " << ulTrChId
                  << std::endl;
    }
    UmtsCphyTrChConfigReqInfo trCfgInfo;
    trCfgInfo.trChDir = UMTS_CH_UPLINK;
    trCfgInfo.trChId = (unsigned char)ulTrChId;
    trCfgInfo.trChType = rbSetupPara.transFormat.trChType;
    trCfgInfo.ueId = node->nodeId;
    trCfgInfo.logChPrio = rbSetupPara.logPriority;
    memcpy(&(trCfgInfo.transFormat),
        &(rbSetupPara.transFormat),
        sizeof(UmtsTransportFormat));

    UmtsLayer3ConfigTrCh(node, umtsL3, &trCfgInfo);

    char rabId = rbSetupPara.rabId;
    UmtsLayer3CnDomainId domain = rbSetupPara.domain;
    char rbId = rbSetupPara.rbId;
    if (DEBUG_RR)
    {
        std::cout << "IE RAB Info to Setup: "
                  << "RAB ID: " << (int)rabId
                  << "  RB ID: " << (int)rbId
                  << std::endl;
    }
    UmtsUeRabInfo* rabInfo = (UmtsUeRabInfo*)
                              MEM_malloc(sizeof(UmtsUeRabInfo));
    rabInfo->cnDomainId = domain;

    for (int i = 1; i < UMTS_MAX_RB_PER_RAB; i++)
    {
        rabInfo->rbIds[i] = UMTS_INVALID_RB_ID;
    }
    rabInfo->rbIds[0] = rbId;
    ueRrc->rabInfos[(int)rabId] = rabInfo;
    ueRrc->rbRabId[rbId - UMTS_RBINRAB_START_ID] = rabId;

    char logPriority = rbSetupPara.logPriority;
    UmtsCmacConfigReqRbInfo rbInfo;

    rbInfo.chDir = UMTS_CH_DOWNLINK;
    rbInfo.logChId = rbSetupPara.ulDtchId;
    rbInfo.logChType = UMTS_LOGICAL_DTCH;
    rbInfo.logChPrio = logPriority;
    rbInfo.rbId = rbId;
    rbInfo.trChType = trCfgInfo.trChType;
    rbInfo.ueId = node->nodeId;
    rbInfo.trChId = trCfgInfo.trChId;
    rbInfo.releaseRb = FALSE;
    UmtsLayer3ConfigRadioBearer(
         node,
         umtsL3,
         &rbInfo,
         NULL,
         NULL,
         0);

    //Configure upLink
    rbInfo.chDir = UMTS_CH_UPLINK;
    UmtsPhysicalChannelType ulPhChType = UMTS_PHYSICAL_DPDCH;
    UmtsLayer3ConfigRadioBearer(
         node,
         umtsL3,
         &rbInfo,
         &ulPhChType,
         &ulPhChId,
         1,
         &(rbSetupPara.transFormat));

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
            rbInfo.ueId);

        UmtsLayer3ConfigRlcEntity(
            node,
            umtsL3,
            rbId,
            UMTS_RLC_ENTITY_RX,
            UMTS_RLC_ENTITY_TM,
            &rbSetupPara.rlcConfig.tmConfig,
            rbInfo.ueId);
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
            rbInfo.ueId);
        UmtsLayer3ConfigRlcEntity(
            node,
            umtsL3,
            rbId,
            UMTS_RLC_ENTITY_RX,
            UMTS_RLC_ENTITY_UM,
            &rbSetupPara.rlcConfig.umConfig,
            rbInfo.ueId);
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
            rbInfo.ueId);
    }
    else
    {
        ERROR_ReportError("UmtsLayer3NodebHandleNbapRbSetup: "
            "receives wrong RLC entity type to setup ");
    }

    std::string msgBuf;
    msgBuf += rbId;
    msgBuf += rabId;
    Message* repMsg =
        MESSAGE_Alloc(node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        repMsg,
                        (int)msgBuf.size(),
                        TRACE_UMTS_LAYER3);
    memcpy(MESSAGE_ReturnPacket(repMsg),
           msgBuf.data(),
           msgBuf.size());
    UmtsLayer3AddHeader(
        node,
        repMsg,
        UMTS_LAYER3_PD_RR,
        0,
        (char)UMTS_RR_MSG_TYPE_RADIO_BEARER_SETUP_COMPLETE);
    UmtsLayer3UeSendPacketToLayer2(node,
                                   umtsL3->interfaceIndex,
                                   UMTS_SRB2_ID,
                                   repMsg);
}
// /**
// FUNCTION   :: UmtsLayer3UeHandleRbSetup
// LAYER      :: Layer3 RRC
// PURPOSE    :: Handle RADIO_BEARER_SETUP message
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + transctId : char         : transaction id
// + msg       : Message*     : RRC message.
// RETURN     :: NULL
// **/
static
void UmtsLayer3UeHandleRbSetup(
        Node* node,
        UmtsLayer3Data *umtsL3,
        char transctId,
        Message* msg)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*) umtsL3->dataPtr;
    char* packet = MESSAGE_ReturnPacket(msg);
    int index = 0;

    if (DEBUG_RR)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        std::cout << "UE: " << node->nodeId
                  << " received an RADIO_BEARER_SETUP"
                  << " message from RNC" << std::endl;
    }

    UmtsUeRbSetupPara rbSetupPara;
    memcpy(&rbSetupPara, packet, sizeof(rbSetupPara));
    index += sizeof(UmtsUeRbSetupPara);

    // update stat
    ueL3->stat.numRadioBearerSetupRcvd ++;

    if (rbSetupPara.hsdpaRb)
    {
        UmtsLayer3UeHandleHsdpaRbSetup(
            node, umtsL3, transctId, msg);
    }
    else
    {
        UmtsLayer3UeHandleRegularRbSetup(
            node, umtsL3, transctId, msg);
    }
}
// /**
// FUNCTION   :: UmtsLayer3UeHandleHsdpaRbRelease
// LAYER      :: Layer3 RRC
// PURPOSE    :: Handle RADIO_BEARER_RELEASE message
// PARAMETERS ::
// + node      : Node*              : Pointer to node.
// + umtsL3    : UmtsLayer3Data *   : Pointer to UMTS Layer3 data
// + transctId : char               : transaction id
// + msg       : Message*           : RRC message.
// RETURN     :: NULL
// **/
static
void UmtsLayer3UeHandleHsdpaRbRelease(
        Node* node,
        UmtsLayer3Data *umtsL3,
        char transctId,
        Message* msg)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*) umtsL3->dataPtr;
    UmtsRrcUeData* ueRrc = ueL3->rrcData;
    char* packet = MESSAGE_ReturnPacket(msg);
    int index = 0;

    if (DEBUG_RR || DEBUG_RB_HSDPA)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n");
        std::cout << "UE: " << node->nodeId
                  << " received an HSDPA RADIO_BEARER_RELEASE"
                  << " message from RNC" << std::endl;
    }

    UmtsUeRbSetupPara rbSetupPara;
    memcpy(&rbSetupPara, packet, sizeof(rbSetupPara));
    index += sizeof(UmtsUeRbSetupPara);

    int numCells = (int)packet[index++];

    // update stats
    //ueL3->stat.numRadioBearerRelRcvd ++;

    // config the PHY chs spread factor
    UmtsUeSetSpreadFactorInUse(
        node, umtsL3->interfaceIndex, rbSetupPara.sfFactor);
    UmtsMacPhyUeSetDpdchNumber(
        node, umtsL3->interfaceIndex, rbSetupPara.numUlDPdch);
    if (DEBUG_RR)
    {
        printf("\nConfigure current SfInUse to %d numUlDPdch %d\n",
            rbSetupPara.sfFactor, rbSetupPara.numUlDPdch);
    }

    for (int j = 0; j < numCells; j++)
    {
        UmtsRrcNodebResInfo cellInfo;
        memcpy(&cellInfo, &packet[index], sizeof(cellInfo));
        index += sizeof(cellInfo);
        int numPhCh = cellInfo.numDlDpdch;
        if (DEBUG_RR || DEBUG_RB_HSDPA)
        {
            std::cout << "Handling IE DL_DPCH_INFO: "
                      << "CELL ID: " << cellInfo.cellId
                      << " Number of channels: "
                      << numPhCh << std::endl;
        }
        for (int i = 0; i < numPhCh; i++)
        {
            if (DEBUG_RR || DEBUG_RB_HSDPA)
            {
                std::cout << "   Spreading Factor: "
                          << cellInfo.sfFactor[i]
                          << "   Channelization code: "
                          << cellInfo.chCode[i]
                          << std::endl;
            }

            UmtsRcvPhChInfo dlDpdchInfo;
            dlDpdchInfo.chCode = cellInfo.chCode[i];
            dlDpdchInfo.sfFactor = cellInfo.sfFactor[i];
            UmtsMacPhyReleaseRcvPhCh(
                node,
                umtsL3->interfaceIndex,
                cellInfo.cellId,
                &dlDpdchInfo);
        }
    }

    if (rbSetupPara.rlSetupNeeded)
    {
        // release HS-DPCCH
        UmtsCphyRadioLinkRelReqInfo relReqInfo;

        relReqInfo.phChDir = UMTS_CH_UPLINK;
        relReqInfo.phChType = UMTS_PHYSICAL_HSDPCCH;
        relReqInfo.phChId = 1;

        UmtsLayer3ReleaseRadioLink(node, umtsL3, &relReqInfo);
    }

    int ulDchId = rbSetupPara.ulDchId;
    if (DEBUG_RR || DEBUG_RB_HSDPA)
    {
        std::cout << "IE UL_TrCh relesed: " << ulDchId
                  << std::endl;
    }
    UmtsCphyTrChReleaseReqInfo trRelInfo;
    trRelInfo.trChDir = UMTS_CH_UPLINK;
    trRelInfo.trChId = (unsigned char)ulDchId;
    trRelInfo.trChType = UMTS_TRANSPORT_HSDSCH;
    UmtsLayer3ReleaseTrCh(node, umtsL3, &trRelInfo);

    char rabId = rbSetupPara.rabId;
    char rbId = rbSetupPara.rbId;
    int ulDtchId = rbSetupPara.ulDtchId;
    if (DEBUG_RR || DEBUG_RB_HSDPA)
    {
        std::cout << "IE RAB Info to Setup: "
                  << "RAB ID: " << (int)rabId
                  << "  RB ID: " << (int)rbId
                  << std::endl;
    }

    //UmtsRlcEntityType rlcType = rbSetupPara.rlcType;

    UmtsCmacConfigReqRbInfo rbInfo;
    rbInfo.chDir = UMTS_CH_UPLINK;
    rbInfo.logChId = (unsigned char)ulDtchId ;
    rbInfo.logChType = UMTS_LOGICAL_DTCH;
    rbInfo.rbId = rbId;
    rbInfo.trChType = trRelInfo.trChType;
    rbInfo.ueId = node->nodeId;
    rbInfo.trChId = trRelInfo.trChId;
    rbInfo.releaseRb = TRUE;
    UmtsLayer3ReleaseRadioBearer(node, umtsL3, &rbInfo);

    //Release uplink entity
    UmtsLayer3ReleaseRlcEntity(node,
                               umtsL3,
                               rbId,
                               UMTS_RLC_ENTITY_BI);

    UmtsUeRabInfo* rabInfo = ueRrc->rabInfos[(int)rabId];
    MEM_free(rabInfo);
    ueRrc->rabInfos[(int)rabId] = NULL;
    ueRrc->rbRabId[rbId - UMTS_RBINRAB_START_ID]
        = UMTS_INVALID_RB_ID;

    std::string msgBuf;
    msgBuf += rbId;
    msgBuf += rabId;
    Message* repMsg =
        MESSAGE_Alloc(node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        repMsg,
                        (int)msgBuf.size(),
                        TRACE_UMTS_LAYER3);
    memcpy(MESSAGE_ReturnPacket(repMsg),
           msgBuf.data(),
           msgBuf.size());
    UmtsLayer3AddHeader(
        node,
        repMsg,
        UMTS_LAYER3_PD_RR,
        0,
        (char)UMTS_RR_MSG_TYPE_RADIO_BEARER_RELEASE_COMPLETE);
    UmtsLayer3UeSendPacketToLayer2(node,
                                   umtsL3->interfaceIndex,
                                   UMTS_SRB2_ID,
                                   repMsg);

}
// /**
// FUNCTION   :: UmtsLayer3UeHandleRegularRbRelease
// LAYER      :: Layer3 RRC
// PURPOSE    :: Handle RADIO_BEARER_RELEASE message
// PARAMETERS ::
// + node      : Node*              : Pointer to node.
// + umtsL3    : UmtsLayer3Data *   : Pointer to UMTS Layer3 data
// + transctId : char               : transaction id
// + msg       : Message*           : RRC message.
// RETURN     :: NULL
// **/
static
void UmtsLayer3UeHandleRegularRbRelease(
        Node* node,
        UmtsLayer3Data *umtsL3,
        char transctId,
        Message* msg)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*) umtsL3->dataPtr;
    UmtsRrcUeData* ueRrc = ueL3->rrcData;
    char* packet = MESSAGE_ReturnPacket(msg);
    int index = 0;

    if (DEBUG_RR)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n");
        std::cout << "UE: " << node->nodeId
                  << " received an RADIO_BEARER_RELEASE"
                  << " message from RNC for regular RB" << std::endl;
    }

    UmtsUeRbSetupPara rbSetupPara;
    memcpy(&rbSetupPara, packet, sizeof(rbSetupPara));
    index += sizeof(UmtsUeRbSetupPara);

    int numCells = (int)packet[index++];

    // config the PHY chs spread factor
    UmtsUeSetSpreadFactorInUse(
        node, umtsL3->interfaceIndex, rbSetupPara.sfFactor);
    UmtsMacPhyUeSetDpdchNumber(
        node, umtsL3->interfaceIndex, rbSetupPara.numUlDPdch);
    if (DEBUG_RR)
    {
        printf("\nConfigure current SfInUse to %d numUlDPdch %d\n",
            rbSetupPara.sfFactor, rbSetupPara.numUlDPdch);
    }

    for (int j = 0; j < numCells; j++)
    {
        UmtsRrcNodebResInfo cellInfo;
        memcpy(&cellInfo, &packet[index], sizeof(cellInfo));
        index += sizeof(cellInfo);
        int numPhCh = cellInfo.numDlDpdch;
        if (DEBUG_RR)
        {
            std::cout << "Handling IE DL_DPCH_INFO: "
                      << "CELL ID: " << cellInfo.cellId
                      << " Number of channels: "
                      << numPhCh << std::endl;
        }
        for (int i = 0; i < numPhCh; i++)
        {
            if (DEBUG_RR)
            {
                std::cout << "   Spreading Factor: "
                          << cellInfo.sfFactor[i]
                          << "   Channelization code: "
                          << cellInfo.chCode[i]
                          << std::endl;
            }

            UmtsRcvPhChInfo dlDpdchInfo;
            dlDpdchInfo.chCode = cellInfo.chCode[i];
            dlDpdchInfo.sfFactor = cellInfo.sfFactor[i];
            UmtsMacPhyReleaseRcvPhCh(
                node,
                umtsL3->interfaceIndex,
                cellInfo.cellId,
                &dlDpdchInfo);
        }
    }

    int ulDchId = rbSetupPara.ulDchId;
    if (DEBUG_RR)
    {
        std::cout << "IE UL_TrCh added: " << ulDchId
                  << std::endl;
    }
    UmtsCphyTrChReleaseReqInfo trRelInfo;
    trRelInfo.trChDir = UMTS_CH_UPLINK;
    trRelInfo.trChId = (unsigned char)ulDchId;
    trRelInfo.trChType = UMTS_TRANSPORT_DCH;
    UmtsLayer3ReleaseTrCh(node, umtsL3, &trRelInfo);

    char rabId = rbSetupPara.rabId;
    char rbId = rbSetupPara.rbId;
    int ulDtchId = rbSetupPara.ulDtchId;
    if (DEBUG_RR)
    {
        std::cout << "IE RAB Info to Setup: "
                  << "RAB ID: " << (int)rabId
                  << "  RB ID: " << (int)rbId
                  << std::endl;
    }

    UmtsCmacConfigReqRbInfo rbInfo;
    rbInfo.chDir = UMTS_CH_UPLINK;
    rbInfo.logChId = (unsigned char)ulDtchId ;
    rbInfo.logChType = UMTS_LOGICAL_DTCH;
    rbInfo.rbId = rbId;
    rbInfo.trChType = trRelInfo.trChType;
    rbInfo.ueId = node->nodeId;
    rbInfo.trChId = trRelInfo.trChId;
    rbInfo.releaseRb = TRUE;
    UmtsLayer3ReleaseRadioBearer(node, umtsL3, &rbInfo);

    //Release uplink entity
    UmtsLayer3ReleaseRlcEntity(node,
                               umtsL3,
                               rbId,
                               UMTS_RLC_ENTITY_BI);

    UmtsUeRabInfo* rabInfo = ueRrc->rabInfos[(int)rabId];
    MEM_free(rabInfo);
    ueRrc->rabInfos[(int)rabId] = NULL;
    ueRrc->rbRabId[rbId - UMTS_RBINRAB_START_ID]
        = UMTS_INVALID_RB_ID;

    std::string msgBuf;
    msgBuf += rbId;
    msgBuf += rabId;
    Message* repMsg =
        MESSAGE_Alloc(node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        repMsg,
                        (int)msgBuf.size(),
                        TRACE_UMTS_LAYER3);
    memcpy(MESSAGE_ReturnPacket(repMsg),
           msgBuf.data(),
           msgBuf.size());
    UmtsLayer3AddHeader(
        node,
        repMsg,
        UMTS_LAYER3_PD_RR,
        0,
        (char)UMTS_RR_MSG_TYPE_RADIO_BEARER_RELEASE_COMPLETE);
    UmtsLayer3UeSendPacketToLayer2(node,
                                   umtsL3->interfaceIndex,
                                   UMTS_SRB2_ID,
                                   repMsg);

}

// /**
// FUNCTION   :: UmtsLayer3UeHandleRbRelease
// LAYER      :: Layer3 RRC
// PURPOSE    :: Handle RADIO_BEARER_RELEASE message
// PARAMETERS ::
// + node      : Node*              : Pointer to node.
// + umtsL3    : UmtsLayer3Data *   : Pointer to UMTS Layer3 data
// + transctId : char               : transaction id
// + msg       : Message*           : RRC message.
// RETURN     :: NULL
// **/
static
void UmtsLayer3UeHandleRbRelease(
        Node* node,
        UmtsLayer3Data *umtsL3,
        char transctId,
        Message* msg)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*) umtsL3->dataPtr;
    char* packet = MESSAGE_ReturnPacket(msg);
    int index = 0;

    if (DEBUG_RR)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n");
        std::cout << "UE: " << node->nodeId
                  << " received an RADIO_BEARER_RELEASE"
                  << " message from RNC" << std::endl;
    }

    UmtsUeRbSetupPara rbSetupPara;
    memcpy(&rbSetupPara, packet, sizeof(rbSetupPara));
    index += sizeof(UmtsUeRbSetupPara);

    // update stats
    ueL3->stat.numRadioBearerRelRcvd ++;

    if (rbSetupPara.hsdpaRb)
    {
        UmtsLayer3UeHandleHsdpaRbRelease(
            node, umtsL3, transctId, msg);
    }
    else
    {
        UmtsLayer3UeHandleRegularRbRelease(
            node, umtsL3, transctId, msg);
    }
}

// /**
// FUNCTION   :: UmtsLayer3UeHandleActvSetUp
// LAYER      :: Layer3 RRC
// PURPOSE    :: Handle active Set setup message
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + transctId : char         : transaction id
// + msg       : Message*     : RRC message.
// RETURN     :: NULL
// **/
static
void UmtsLayer3UeHandleActvSetUp(
        Node* node,
        UmtsLayer3Data *umtsL3,
        char transctId,
        Message* msg)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*) umtsL3->dataPtr;
    char* packet = MESSAGE_ReturnPacket(msg);
    int index = 0;

    // update stat
    ueL3->stat.numActiveSetSetupRcvd ++;
    BOOL addCell = (BOOL)packet[index++];
    UInt32 cellId;
    memcpy(&cellId, &packet[index], sizeof(cellId));
    index += sizeof(cellId);
    if (DEBUG_HO)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n");
        std::cout << "UE: " << node->nodeId
                  << " receives an ACTIVE SET UPDATE message: "
                  << "for CELL: " << cellId;
        if (addCell)
        {
            std::cout << " to add cell" << std::endl;
        }
        else
        {
            std::cout << " to remove cell" << std::endl;
        }
    }
    if (addCell)
    {
        UmtsRcvPhChInfo dlDpdchInfo;
        memcpy(&dlDpdchInfo.sfFactor,
               &packet[index],
               sizeof(UmtsSpreadFactor));
        index += sizeof(UmtsSpreadFactor);
        memcpy(&dlDpdchInfo.chCode,
               &packet[index],
               sizeof(UInt32));
        index += sizeof(UInt32);
        if (DEBUG_HO)
        {
            std::cout << "   Spreading Factor: "
                      << dlDpdchInfo.sfFactor
                      << "   Channelization code: "
                      << dlDpdchInfo.chCode
                      << std::endl;
        }
        UmtsMacPhyConfigRcvPhCh(
            node,
            umtsL3->interfaceIndex,
            cellId,
            &dlDpdchInfo);
        int numRbs;
        numRbs = (int) packet[index ++];
        while (index < MESSAGE_ReturnPacketSize(msg))
        {
            char rbId;
            rbId= packet[index ++];
            char numDpdch = packet[index ++];
            for (int i = 0; i < numDpdch; i++)
            {
                UmtsRcvPhChInfo dlDpdchInfo;
                memcpy(&dlDpdchInfo.sfFactor,
                       &packet[index],
                       sizeof(UmtsSpreadFactor));
                index += sizeof(UmtsSpreadFactor);
                memcpy(&dlDpdchInfo.chCode,
                       &packet[index],
                       sizeof(UInt32));
                index += sizeof(UInt32);
                if (DEBUG_HO)
                {
                    std::cout << "   Spreading Factor: "
                              << dlDpdchInfo.sfFactor
                              << "   Channelization code: "
                              << dlDpdchInfo.chCode
                              << std::endl;
                }
                UmtsMacPhyConfigRcvPhCh(
                    node,
                    umtsL3->interfaceIndex,
                    cellId,
                    &dlDpdchInfo);
            }
        }
        std::list<UmtsLayer3UeNodebInfo*>::iterator it;
        for (it = ueL3->ueNodeInfo->begin();
             it != ueL3->ueNodeInfo->end();
             it ++)
        {
            if ((*it)->cellId == cellId)
            {
                (*it)->cellStatus = UMTS_CELL_ACTIVE_SET;

                // GUI
                if (node->guiOption)
                {
                    GUI_SendIntegerData(node->nodeId,
                                        ueL3->stat.activeSetSizeGuiId,
                                        UmtsLayer3UeGetCellStatusSetSize(
                                           node, umtsL3, ueL3,
                                           UMTS_CELL_ACTIVE_SET),
                                        node->getNodeTime());
                }

                break;
            }
        }
    }
    else
    {
        UmtsMacPhyReleaseRcvPhCh(
            node,
            umtsL3->interfaceIndex,
            cellId);
        std::list<UmtsLayer3UeNodebInfo*>::iterator it;
        for (it = ueL3->ueNodeInfo->begin();
            it != ueL3->ueNodeInfo->end();
            it ++)
        {
            if ((*it)->cellId == cellId)
            {
                (*it)->cellStatus = UMTS_CELL_MONITORED_SET;

                // GUI
                if (node->guiOption)
                {
                    GUI_SendIntegerData(node->nodeId,
                                        ueL3->stat.activeSetSizeGuiId,
                                        UmtsLayer3UeGetCellStatusSetSize(
                                           node, umtsL3, ueL3,
                                           UMTS_CELL_ACTIVE_SET),
                                        node->getNodeTime());
                }
                break;
            }
        }
    }

    UmtsLayer3UeSendActvSetUpComp(
            node,
            umtsL3,
            ueL3,
            cellId,
            addCell);

    // update stat
    ueL3->stat.numActiveSetupCompSent ++;
}

// /**
// FUNCTION   :: UmtsLayer3UeHandleSignalConnRel
// LAYER      :: Layer3 RRC
// PURPOSE    :: Handle RADIO_BEARER_SETUP message
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + transctId : char         : transaction id
// + msg       : Message*     : RRC message.
// RETURN     :: NULL
// **/
static
void UmtsLayer3UeHandleSignalConnRel(
        Node* node,
        UmtsLayer3Data *umtsL3,
        char transctId,
        Message* msg)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*) umtsL3->dataPtr;
    UmtsRrcUeData* ueRrc = ueL3->rrcData;
    char* packet = MESSAGE_ReturnPacket(msg);

    UmtsLayer3CnDomainId domain = (UmtsLayer3CnDomainId)(*packet);

    // update stats
    ueL3->stat.numSigConnRelRcvd ++;

    if (ueRrc->signalConn[domain])
    {
        ueRrc->signalConn[domain] = FALSE;
        UmtsLayer3UeSigConnRelInd(node,
                                  umtsL3,
                                  ueL3,
                                  domain);
    }
}

//
// /**
// FUNCTION   :: UmtsLayer3UeHandleMasterSib
// LAYER      :: Layer3
// PURPOSE    :: Handle RR SIB type 3 msg.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data*   : Pointer to L3 data
// + ueL3             : UmtsLayer3Ue *    : Pointer to l3 Ue data
// + mibInfo          : UmtsMasterInfoBlock* : sib Message to interpret.
// RETURN     :: NULL
static
BOOL UmtsLayer3UeHandleMasterSib(Node* node,
                                 UmtsLayer3Data* umtsL3,
                                 UmtsLayer3Ue* ueL3,
                                 UmtsMasterInfoBlock* mibInfo)
{
    UmtsLayer3UeNodebInfo* nodebInfo;
    // handle master info block
    if (ueL3->plmnType != mibInfo->plmnType)
    {
        // this UE does not support this PLMN type
        return FALSE;
    }

    nodebInfo = UmtsLayer3UeGetNodeBData(
                    node, umtsL3, ueL3, mibInfo->cellId);
    if (!nodebInfo)
    {
        // not found the cell
        return FALSE;
    }

    if (ueL3->rrcData->subState >
        UMTS_RRC_SUBSTATE_SUITABLE_CELL_SELECTED &&
        nodebInfo->cellId != ueL3->rrcData->selectedCellId)
        //nodebInfo->cellStatus != UMTS_CELL_ACTIVE_SET)
    {
        // already select a cell and this one is not from cell in active set
        // discard it
        return FALSE;
    }

    // only state less  than  cell_selected
    // or cell is in active set should be here
    if (nodebInfo->cellStatus == UMTS_CELL_ACTIVE_SET &&
        nodebInfo->sibInd == 0xFFFF &&
        nodebInfo->valueTag == mibInfo->valueTag)
    {
        // all the sib for this active cell has been seen before
        return FALSE;
    }

    if (nodebInfo->valueTag != mibInfo->valueTag)
    {
        nodebInfo->sibInd = 0xFFE8; // need to receive a new serial of SIB
        nodebInfo->valueTag = mibInfo->valueTag;
    }

    if (ueL3->rrcData->subState == UMTS_RRC_SUBSTATE_NO_PLMN)
    {

        // determine the suitable cell/strongest cell
        double strongestRscp = ueL3->para.Qrxlevmin;
        std::list <UmtsLayer3UeNodebInfo*>::iterator it;
        std::list <UmtsLayer3UeNodebInfo*>::iterator itStrongest;
        clocktype curTime = node->getNodeTime();

        itStrongest = ueL3->ueNodeInfo->end();
        for (it = ueL3->ueNodeInfo->begin();
            it != ueL3->ueNodeInfo->end();
            it ++)
        {
            (*it)->cpichRscp =  (*it)->cpichRscpMeas->GetAvgMeas(curTime);
            if (strongestRscp < (*it)->cpichRscp)
            {
                strongestRscp = (*it)->cpichRscp;
                itStrongest = it;
            }
        }

        // move this nodeB to active set
        // FDD here only RSCP Rx Level is used
        if (itStrongest != ueL3->ueNodeInfo->end()
            && (*itStrongest)->cpichRscp > ueL3->para.Qrxlevmin)
        {
            // for init cell search only
            ueL3->rrcData->subState = UMTS_RRC_SUBSTATE_PLMN_SELECTED;
            if (ueL3->mmData->idleSubState ==
                UMTS_MM_MS_IDLE_NO_CELL_AVAILABLE)
            {
                ueL3->mmData->idleSubState = UMTS_MM_MS_IDLE_PLMN_SEARCH;
            }

            // currently only the strongest one is in active set
            //(*itStrongest)->cellStatus = UMTS_CELL_ACTIVE_SET;
            (*itStrongest)->plmnId = mibInfo->plmnId;
            (*itStrongest)->valueTag = mibInfo->valueTag;
            (*itStrongest)->sibInd = 0xFFE8; // need to receive sib1,2,3,5
            ueL3->rrcData->subState =
                UMTS_RRC_SUBSTATE_SUITABLE_CELL_SELECTED;
            ueL3->rrcData->selectedCellId = (*itStrongest)->cellId;
            ueL3->rrcData->counterV300 = 0;
            ueL3->rrcData->counterV308 = 0;

            // keep a copy to the primary nodeB/cell
            ueL3->priNodeB = *itStrongest;

            // allocate space for other info
            //(*itStrongest)->hoCellSelectionParam =
            //    (UmtsLayer3UePara*) MEM_malloc(sizeof(UmtsLayer3UePara));

            // todo allocalte for PRACH
            //mem(*itStrongest)->hoCellSelectionParam
            if (DEBUG_RR || DEBUG_CELL_SELECT)
            {
                UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
                printf("\n\tselect a suitable Cell %d "
                       "and ready to receive other types of system info\n",
                       (*itStrongest)->cellId);
            }
        }
    }

    return TRUE;
}

//
// /**
// FUNCTION   :: UmtsLayer3UeHandleSibType1
// LAYER      :: Layer3
// PURPOSE    :: Handle RR SIB type 1 msg.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data*   : Pointer to L3 data
// + ueL3             : UmtsLayer3Ue *    : Pointer to l3 Ue data
// + cellId           : UInt32            : cellId
// + sibType1         : UmtsSibType1*     : sib Message to interpret.
// RETURN     :: void : NULL
static
void UmtsLayer3UeHandleSibType1(Node* node,
                                UmtsLayer3Data* umtsL3,
                                UmtsLayer3Ue* ueL3,
                                UInt32 cellId,
                                UmtsSibType1* sibType1)
{
    // Get the nodeB Info
    UmtsLayer3UeNodebInfo* nodebInfo;

    nodebInfo = UmtsLayer3UeGetNodeBData(
                    node, umtsL3, ueL3, cellId);
    if (!nodebInfo)
    {
        // not found the cell
        return ;
    }

    if (!(nodebInfo->sibInd & 0x0001))
    {
        // first time get the SIB type1
        // need to handle it
        nodebInfo->sibInd |= 0x0001;
        if (DEBUG_RR || DEBUG_CELL_SELECT)
        {
            UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
            printf("node %d (L3-RR): receive SIB type1 from cell %d\n",
                   node->nodeId, cellId);
        }
    }

    if (ueL3->rrcData->state == UMTS_RRC_STATE_IDLE
        && ueL3->rrcData->subState ==
        UMTS_RRC_SUBSTATE_SUITABLE_CELL_SELECTED
        && nodebInfo->cellId == ueL3->rrcData->selectedCellId
        && nodebInfo->sibInd == 0xFFFF)
    {
        if (DEBUG_MM)
        {
            printf("\tnode %d (L3-MM):after rcvd SIB1, "
                "ready to Loc update\n",
                node->nodeId);
        }
        // ready to camp on active cell
        ueL3->rrcData->subState = UMTS_RRC_SUBSTATE_CAMPED_NORMALLY;
        UmtsMmUeUpdateMmState(
            node, umtsL3, ueL3, UMTS_MM_UPDATE_CellSelected);
    }
}

// /**
// FUNCTION   :: UmtsLayer3UeHandleSibType2
// LAYER      :: Layer3
// PURPOSE    :: Handle RR SIB type 3 msg.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data*   : Pointer to L3 data
// + ueL3             : UmtsLayer3Ue *    : Pointer to l3 Ue data
// + cellId           : UInt32            : cellId
// + sibType2         : UmtsSibType3*     : sib Message to interpret.
// RETURN     :: void : NULL
static
void UmtsLayer3UeHandleSibType2(Node* node,
                                UmtsLayer3Data* umtsL3,
                                UmtsLayer3Ue* ueL3,
                                UInt32 cellId,
                                UmtsSibType2* sibType2)
{
    // Get the nodeB Info
    UmtsLayer3UeNodebInfo* nodebInfo;

    nodebInfo = UmtsLayer3UeGetNodeBData(
                    node, umtsL3, ueL3, cellId);
    if (!nodebInfo)
    {
        // not found the cell
        return ;
    }

    if (!(nodebInfo->sibInd & 0x0002))
    {
        // first time get the SIB type2
        // need to handle it
        nodebInfo->regAreaId = sibType2->regAreaId;
        nodebInfo->sibInd |= 0x0002;
        if (DEBUG_RR || DEBUG_CELL_SELECT)
        {
            UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
            printf("node %d (L3-RR): receive SIB type2 from cell %d\n",
                   node->nodeId, cellId);
        }
    }

    if (ueL3->rrcData->state == UMTS_RRC_STATE_IDLE
        && ueL3->rrcData->subState ==
        UMTS_RRC_SUBSTATE_SUITABLE_CELL_SELECTED
        && nodebInfo->cellId == ueL3->rrcData->selectedCellId
        && nodebInfo->sibInd == 0xFFFF)
    {
        if (DEBUG_MM)
        {
            printf("\tnode %d (L3-MM):after rcvd SIB2, "
                "ready to Loc update\n",
                node->nodeId);
        }

        // ready to camp on active cell
        ueL3->rrcData->subState = UMTS_RRC_SUBSTATE_CAMPED_NORMALLY;
        UmtsMmUeUpdateMmState(
            node, umtsL3, ueL3, UMTS_MM_UPDATE_CellSelected);
    }
}

// /**
// FUNCTION   :: UmtsLayer3UeHandleSibType3
// LAYER      :: Layer3
// PURPOSE    :: Handle RR SIB type 3 msg.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data*   : Pointer to L3 data
// + ueL3             : UmtsLayer3Ue *    : Pointer to l3 Ue data
// + cellId           : UInt32            : cell Id
// + sibType3         : UmtsSibType3*     : sib Message to interpret.
// RETURN     :: void : NULL
static
void UmtsLayer3UeHandleSibType3(Node* node,
                                UmtsLayer3Data* umtsL3,
                                UmtsLayer3Ue* ueL3,
                                UInt32 cellId,
                                UmtsSibType3* sibType3)
{
    // Get the nodeB Info
    UmtsLayer3UeNodebInfo* nodebInfo;

    nodebInfo = UmtsLayer3UeGetNodeBData(
                    node, umtsL3, ueL3, cellId);
    if (!nodebInfo)
    {
        // not found the cell
        return ;
    }

    if (!(nodebInfo->sibInd & 0x0004))
    {
        // first time get the SIB type3
        // need to handle it
        nodebInfo->sibInd |= 0x0004;

        // TODO
        // Get all the params and set to nodebL3->para.XXXX
        // Currently it is read from configuration
        // TODO: get UL ch from SIB, now 0 is used as default
        UmtsSetUlChIndex(
            node, umtsL3->interfaceIndex, sibType3->ulChIndex);
        if (DEBUG_RR || DEBUG_CELL_SELECT)
        {
            UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
            printf("node %d (L3-RR): receive SIB type3 from cell %d\n",
                   node->nodeId, cellId);
        }
    }

    if (ueL3->rrcData->state == UMTS_RRC_STATE_IDLE
        && ueL3->rrcData->subState ==
        UMTS_RRC_SUBSTATE_SUITABLE_CELL_SELECTED
        && nodebInfo->cellId == ueL3->rrcData->selectedCellId
        && nodebInfo->sibInd == 0xFFFF)
    {
        if (DEBUG_MM)
        {
            printf("\tnode %d (L3-MM): after rcvd SIB3, "
                "ready to Loc update\n",
                node->nodeId);
        }
        // ready to camp on active cell
        ueL3->rrcData->subState = UMTS_RRC_SUBSTATE_CAMPED_NORMALLY;
        UmtsMmUeUpdateMmState(
            node, umtsL3, ueL3, UMTS_MM_UPDATE_CellSelected);
    }
}
// /**
// FUNCTION   :: UmtsLayer3UeHandleSibType5
// LAYER      :: Layer3
// PURPOSE    :: Handle RR SIB type 5 msg.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data*   : Pointer to L3 data
// + ueL3             : UmtsLayer3Ue *    : Pointer to l3 Ue data
// + cellId           : UInt32            : cell Id
// + sibType5         : UmtsSibType5 *    : sib Message to interpret.
// RETURN     :: void : NULL
static
void UmtsLayer3UeHandleSibType5(Node* node,
                                UmtsLayer3Data* umtsL3,
                                UmtsLayer3Ue *ueL3,
                                UInt32 cellId,
                                UmtsSibType5 *sibType5)
{
    // Get the nodeB Info
    UmtsLayer3UeNodebInfo* nodebInfo;

    nodebInfo = UmtsLayer3UeGetNodeBData(
                    node, umtsL3, ueL3, cellId);
    if (!nodebInfo)
    {
        // not found the cell
        return ;
    }

    if (!(nodebInfo->sibInd & 0x0010))
    {
        // first time get the SIB type5
        // need to handle it
        nodebInfo->sibInd |= 0x0010;

        if (DEBUG_RR || DEBUG_CELL_SELECT)
        {
            UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
            printf("node %d (L3-RR): receive SIB type5 from cell %d\n",
                   node->nodeId, cellId);
        }

        // TODO
        // Get all the configurations of Phy Chs
        // (PICH,AICH, PCCPCH,SCCPCH,PRACH
        // currently all these information are set for testing purpose
    }

    if (ueL3->rrcData->state == UMTS_RRC_STATE_IDLE
        && ueL3->rrcData->subState ==
        UMTS_RRC_SUBSTATE_SUITABLE_CELL_SELECTED
        && nodebInfo->cellId == ueL3->rrcData->selectedCellId
        && nodebInfo->sibInd == 0xFFFF)
    {
        if (DEBUG_MM)
        {
            printf("\tnode %d (L3-MM): after rcvd SIB5, "
                "ready to Loc update\n",
                node->nodeId);
        }
        // ready to camp on active cell
        ueL3->rrcData->subState = UMTS_RRC_SUBSTATE_CAMPED_NORMALLY;
        UmtsMmUeUpdateMmState(
            node, umtsL3, ueL3, UMTS_MM_UPDATE_CellSelected);
    }
}

// /**
// FUNCTION   :: UmtsLayer3UeHandleSibMsg
// LAYER      :: Layer3
// PURPOSE    :: Handle RR msg.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data*   : Pointer to L3 data
// + ueL3             : UmtsLayer3Ue *    : Pointer to l3 Ue data
// + msg              : Message*          : sib Message to interpret.
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeHandleSibMsg(Node* node,
                              UmtsLayer3Data* umtsL3,
                              UmtsLayer3Ue *ueL3,
                              Message* msg)
{
    if (DEBUG_RR  && 0)
    {
        printf("node %d  (L3-RR): handle SIB msg\n",node->nodeId);
    }

    char *sibPkt;
    int  curPos = 0;
    UmtsMasterInfoBlock  mibInfo;
    UmtsSibType sibType;


    // update stat
    ueL3->stat.numMibRcvd ++;

    sibPkt = (char*) MESSAGE_ReturnPacket(msg);
    curPos ++; // sibPkt + curPos // SFN
    curPos ++; // combination
    curPos ++; // master info block
    memcpy((char*)&mibInfo,
            (char*)(sibPkt + curPos),
            sizeof(UmtsMasterInfoBlock));
    curPos += sizeof(UmtsMasterInfoBlock);

    if (!UmtsLayer3UeHandleMasterSib(node, umtsL3, ueL3, &mibInfo))
    {
        return;
    }

    sibType = (UmtsSibType)*(sibPkt+ curPos);
    curPos ++;

    //  TODO
    // switch the sib type
    switch (sibType)
    {
        case UMTS_SIB_TYPE1:
        {
            UmtsSibType1 sibType1;

            ueL3->stat.numSib1Rcvd ++;
            memcpy((char*) &sibType1,
                    (char*)(sibPkt + curPos),
                    sizeof(UmtsSibType1));
            UmtsLayer3UeHandleSibType1(
                node, umtsL3, ueL3, mibInfo.cellId, &sibType1);
            break;
        }
        case UMTS_SIB_TYPE2:
        {
            UmtsSibType2 sibType2;

            ueL3->stat.numSib2Rcvd ++;
            memcpy((char*) &sibType2,
                    (char*)(sibPkt + curPos),
                    sizeof(UmtsSibType2));
            UmtsLayer3UeHandleSibType2(
                node, umtsL3, ueL3, mibInfo.cellId, &sibType2);
            break;
        }
        case UMTS_SIB_TYPE3:
        {
            UmtsSibType3 sibType3;

            ueL3->stat.numSib3Rcvd ++;
            memcpy((char*) &sibType3,
                    (char*)(sibPkt + curPos),
                    sizeof(UmtsSibType3));
            UmtsLayer3UeHandleSibType3(
                node, umtsL3, ueL3, mibInfo.cellId, &sibType3);
            break;
        }
        case UMTS_SIB_TYPE5:
        {
            UmtsSibType5 sibType5;

            ueL3->stat.numSib5Rcvd ++;
            memcpy((char*) &sibType5,
                    (char*)(sibPkt + curPos),
                    sizeof(UmtsSibType5));
            UmtsLayer3UeHandleSibType5(
                node, umtsL3, ueL3, mibInfo.cellId, &sibType5);
            break;
        }
        default:
        {
            char errStr[MAX_STRING_LENGTH];
            sprintf(errStr,
                    "UMTS Layer3: Node%d(UE) receivers a SIB"
                    "msg from cell %d with unknown type %d",
                    node->nodeId,
                    mibInfo.cellId,
                    sibType);
            ERROR_ReportWarning(errStr);

            break;
        }
    }
    if (DEBUG_RR && 0)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("rcvd a SIB from %d type %d\n", mibInfo.cellId, sibType);
    }
}

// /**
// FUNCTION   :: UmtsLayer3UeHandleRrMsg
// LAYER      :: Layer3
// PURPOSE    :: Handle RR msg.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : interface index
// + msgType          : char              : Radio resource control msg type
// + transactId       : unsigned char     : Message transation Id
// + msg              : Message*          : Message for node to interpret.
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeHandleRrMsg(Node* node,
                             int interfaceIndex,
                             char msgType,
                             unsigned char transactId,
                             Message* msg)
{
    UmtsRRMessageType rrMsgType;
    rrMsgType = (UmtsRRMessageType) msgType;

    UmtsLayer3Data* umtsL3;
    UmtsLayer3Ue *ueL3;

    umtsL3 = UmtsLayer3UeGetData(node);
    ueL3 = (UmtsLayer3Ue*) umtsL3->dataPtr;

    if (rrMsgType != UMTS_RR_MSG_TYPE_SYSTEM_INFORMATION_BCH &&
        ueL3->rrcData->subState < UMTS_RRC_SUBSTATE_SUITABLE_CELL_SELECTED)
    {
        MESSAGE_Free(node, msg);
        return;
    }

    switch (rrMsgType)
    {
        case UMTS_RR_MSG_TYPE_SYSTEM_INFORMATION_BCH:
        {
            UmtsLayer3UeHandleSibMsg(node, umtsL3, ueL3, msg);
            break;
        }
        case UMTS_RR_MSG_TYPE_RRC_CONNECTION_SETUP:
        {
            UmtsLayer3UeHandleRrcConnSetup(
                node,
                umtsL3,
                transactId,
                msg);
            break;
        }
        case UMTS_RR_MSG_TYPE_RRC_CONNECTION_REJECT:
        {
            UmtsLayer3UeHandleRrcConnRej(
                node,
                umtsL3,
                transactId,
                msg);
            break;
        }
        case UMTS_RR_MSG_TYPE_RRC_CONNECTION_RELEASE:
        {
            UmtsLayer3UeHandleRrcConnRel(
                node,
                umtsL3,
                transactId,
                msg);
            break;
        }
        case UMTS_RR_MSG_TYPE_DL_DIRECT_TRANSFER:
        {
            UmtsLayer3UeHandleDlDirectTransfer(
                node,
                umtsL3,
                transactId,
                msg);
            break;
        }
        case UMTS_RR_MSG_TYPE_PAGING_TYPE1:
        case UMTS_RR_MSG_TYPE_PAGING_TYPE2:
        {
            UmtsLayer3UeHandlePaging(
                node,
                umtsL3,
                msg);
            break;
        }
        case UMTS_RR_MSG_TYPE_RADIO_BEARER_SETUP:
        {
            UmtsLayer3UeHandleRbSetup(
                node,
                umtsL3,
                transactId,
                msg);
            break;
        }
        case UMTS_RR_MSG_TYPE_RADIO_BEARER_RELEASE:
        {
            UmtsLayer3UeHandleRbRelease(
                node,
                umtsL3,
                transactId,
                msg);
            break;
        }
        case UMTS_RR_MSG_TYPE_ACTIVE_SET_UPDATE:
        {
            UmtsLayer3UeHandleActvSetUp(
                node,
                umtsL3,
                transactId,
                msg);
            break;
        }
        case UMTS_RR_MSG_TYPE_SIGNALLING_CONNECTION_RELEASE:
        {
            UmtsLayer3UeHandleSignalConnRel(
                node,
                umtsL3,
                transactId,
                msg);
            break;
        }
        default:
        {
            char errStr[MAX_STRING_LENGTH];
            sprintf(errStr,
                    "UMTS Layer3: Node%d(UE) receivers a RR"
                    "msg with unknown type %d",
                    node->nodeId,
                    rrMsgType);
            ERROR_ReportWarning(errStr);
            break;
        }
    }
    MESSAGE_Free(node, msg);
}

//
// handle Measurement
//
// /**
// FUNCTION   :: UmtsLayer3UeReportMeasurement
// LAYER      :: Layer3
// PURPOSE    :: Handle measurement msg.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data*   : Pointer to L3 data
// + ueL3             : UmtsLayer3Ue *    : Pointer to l3 Ue data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeReportMeasurement(
          Node* node,
          UmtsLayer3Data* umtsL3,
          UmtsLayer3Ue *ueL3)
{
    // upate detect list, active lsit and monitor list
    std::list <UmtsLayer3UeNodebInfo*>::iterator it;
    clocktype curTime = node->getNodeTime();

    for (it = ueL3->ueNodeInfo->begin();
        it != ueL3->ueNodeInfo->end();
        it ++)
    {
        // update the measurement
        (*it)->cpichRscp = (*it)->cpichRscpMeas->GetAvgMeas(curTime);
    }
    if (DEBUG_MEASUREMENT && 0)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("report measurement to RNC\n");
    }

    // send MeaurementReport
    UmtsLayer3UeSendMeasurementReport(node, umtsL3, ueL3);

    // upate stat
    ueL3->stat.numMeasReportSent ++;
}


// /**
// FUNCTION   :: UmtsLayer3UeHandleCpichMeasurement
// LAYER      :: Layer3
// PURPOSE    :: Handle measurement msg.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data*   : Pointer to L3 data
// + ueL3             : UmtsLayer3Ue *    : Pointer to l3 Ue data
// + cmd              : void   *          : measurement report to interpret.
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeHandleCpichMeasurement(
          Node* node,
          UmtsLayer3Data* umtsL3,
          UmtsLayer3Ue *ueL3,
          void* cmd)
{
    UmtsCphyMeasIndInfo* measInd;
    UmtsLayer3UeNodebInfo* ueNodeBInfo = NULL;

    measInd = (UmtsCphyMeasIndInfo*) cmd;
    UmtsMeasCpichRscp* rscpVal;
    BOOL needRepMeas = FALSE;
    double bestRscpVal = ueL3->para.Qrxlevmin;

    // update stat
    ueL3->stat.numCpichMeasFromPhyRcvd ++;
    if (measInd->measType == UMTS_MEASURE_L1_CPICH_RSCP)
    {
        rscpVal = (UmtsMeasCpichRscp*)(measInd->measInfo);
    }
    else if (measInd->measType == UMTS_MEASURE_L1_CPICH_Ec_No)
    {
        rscpVal = (UmtsMeasCpichEcNo*)(measInd->measInfo);
    }
    else
    {
        // not support yet
        return;
    }

    // upate detect list, active lsit and monitor list
    std::list<UmtsLayer3UeNodebInfo*>::iterator it;

    for (it = ueL3->ueNodeInfo->begin();
        it != ueL3->ueNodeInfo->end();
        it ++)
    {
        if ((*it)->primaryScCode == rscpVal->priSc)
        {
            // has already been detected
            if (DEBUG_MEASUREMENT)
            {
                UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
                printf("It is a detected cell "
                       "with pri SC code %d in code group %d RxLevel %f\n",
                       (*it)->primaryScCode,
                       (*it)->sscCodeGroupIndex,
                       rscpVal->measVal);
            }
            ueNodeBInfo = *it;

            UmtsMeasElement measElem;
            BOOL priNodeB = FALSE;
            measElem.measVal = rscpVal->measVal;
            measElem.timeStamp = node->getNodeTime();

            // GUI
            if (node->guiOption)
            {
                if (ueL3->priNodeB &&
                    ueL3->priNodeB->primaryScCode ==
                    ueNodeBInfo->primaryScCode)
                {
                    // this is the primary NodeB of the UE
                    priNodeB = TRUE;
                }
            }
            //
            if (measInd->measType == UMTS_MEASURE_L1_CPICH_RSCP)
            {
                ueNodeBInfo->cpichRscpMeas->AddMeasElement(
                     measElem, node->getNodeTime());

                ueNodeBInfo->cpichRscp =
                    ueNodeBInfo->cpichRscpMeas->GetAvgMeas(node->getNodeTime());
                // GUI
                if (node->guiOption && priNodeB)
                {
                    GUI_SendRealData(node->nodeId,
                                     ueL3->stat.priNodeBCpichRscpGuiId,
                                     ueNodeBInfo->cpichRscp,
                                     node->getNodeTime());
                }
            }
            else if (measInd->measType == UMTS_MEASURE_L1_CPICH_Ec_No)
            {
                ueNodeBInfo->cpichEcNoMeas->AddMeasElement(
                     measElem, node->getNodeTime());

                ueNodeBInfo->cpichEcNo =
                    ueNodeBInfo->cpichEcNoMeas->
                     GetAvgMeas(node->getNodeTime());

                // GUI
                if (node->guiOption && priNodeB)
                {
                    double ber;
                    UmtsCodingType codingType = UMTS_CONV_2;
                    UmtsModulationType moduType = UMTS_MOD_QPSK;

                    GUI_SendRealData(node->nodeId,
                                     ueL3->stat.priNodeBCpichEcNoGuiId,
                                     ueNodeBInfo->cpichEcNo,
                                     node->getNodeTime());

                    ber = UmtsMacPhyGetSignalBitErrorRate(
                              node,
                              umtsL3->interfaceIndex,
                              codingType,
                              moduType,
                              rscpVal->measVal);

                    GUI_SendRealData(
                            node->nodeId,
                            ueL3->stat.priNodeBCpichBerGuiId,
                            ber,
                            node->getNodeTime());
                }
                // for Ec_No measurement, just do update
                return;
            }

            // to see if any active cell signal is too low
            if ((*it)->cellStatus == UMTS_CELL_ACTIVE_SET)
            {
                if (measInd->measType == UMTS_MEASURE_L1_CPICH_RSCP &&
                    (*it)->cpichRscp <
                    (ueL3->para.Qrxlevmin + ueL3->para.Qhyst))
                {
                    // if RSCP drops to a value close to min Rx Level
                    // actively report to network
                    if (DEBUG_MEASUREMENT)
                    {
                        UmtsLayer3PrintRunTimeInfo(
                            node, UMTS_LAYER3_SUBLAYER_RRC);
                        printf("\n\tActive cell %d 's RSCP is %f, close "
                               " to threshold %f, need actively report "
                               "measurement to RNC\n",
                               (*it)->cellId, (*it)->cpichRscp,
                               (ueL3->para.Qrxlevmin + ueL3->para.Qhyst));
                    }
                    needRepMeas = TRUE;
                }
            }
        }

        if (measInd->measType == UMTS_MEASURE_L1_CPICH_RSCP)
        {
            // update all NodeBs's RSCP, to find teh best one
            // only RSCP will be here
            (*it)->cpichRscp =
                (*it)->cpichRscpMeas->GetAvgMeas(node->getNodeTime());

            // update the best one
            if (bestRscpVal < (*it)->cpichRscp)
            {
                bestRscpVal = (*it)->cpichRscp;
            }
        }
    }

    if (!ueNodeBInfo)
    {
        // a new detected NodeB, cell
        ueNodeBInfo = new UmtsLayer3UeNodebInfo(
                              rscpVal->priSc, ueL3->para.cellEvalTime);
        ueL3->ueNodeInfo->push_back(ueNodeBInfo);

        UmtsMeasElement measElem;
        measElem.measVal = rscpVal->measVal;
        measElem.timeStamp = node->getNodeTime();

        if (measInd->measType == UMTS_MEASURE_L1_CPICH_RSCP)
        {
            ueNodeBInfo->cpichRscpMeas->AddMeasElement(
                     measElem, node->getNodeTime());
            ueNodeBInfo->cpichRscp =
                ueNodeBInfo->cpichRscpMeas->GetAvgMeas(node->getNodeTime());

            // update the best one
            if (bestRscpVal < rscpVal->measVal)
            {
                bestRscpVal = rscpVal->measVal;
            }

        }
        else if (measInd->measType == UMTS_MEASURE_L1_CPICH_Ec_No)
        {
            ueNodeBInfo->cpichEcNoMeas->AddMeasElement(
                     measElem, node->getNodeTime());
            ueNodeBInfo->cpichEcNo =
                ueNodeBInfo->cpichEcNoMeas->GetAvgMeas(node->getNodeTime());

            return;
        }

        ueNodeBInfo->cellStatus = UMTS_CELL_DETECTED_SET;
        // Assume all DETECTED SET are MONITORED SET
        ueNodeBInfo->cellStatus = UMTS_CELL_MONITORED_SET;

        if (DEBUG_MEASUREMENT)
        {
            UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
            printf("detect a new cell "
                   "with pri SC code %d in code group %d RxLevel %f\n",
                   ueNodeBInfo->primaryScCode,
                   ueNodeBInfo->sscCodeGroupIndex,
                   rscpVal->measVal);
        }
    }

    // to see if cell reselction is needed
    if ((ueL3->rrcData->state == UMTS_RRC_STATE_IDLE &&
        ueL3->rrcData->subState == UMTS_RRC_SUBSTATE_CAMPED_NORMALLY) ||
        (ueL3->rrcData->state == UMTS_RRC_STATE_CONNECTED &&
            ueL3->rrcData->subState != UMTS_RRC_SUBSTATE_CELL_DCH))
    {
        if (ueL3->priNodeB && ueL3->priNodeB == ueNodeBInfo)
        {
            // when in idle and already find one cell
            // or URA_PCH,CELL_PCH,CELL_FACH
            ueL3->priNodeB->cpichRscp =
                ueL3->priNodeB->cpichRscpMeas->GetAvgMeas(node->getNodeTime());
            if (ueL3->priNodeB->cpichRscp < ueL3->para.Ssearch)
            {
                UmtsLayer3UeCheckCellSelectReselect(node, umtsL3, ueL3);
                // TODO: start a timer
                // set evelauation BOOL
                // measure signal
                // after time out
                // make  decision by calling
                // UmtsLayer3UeCheckCellSelectReselect
            }
        }
    }
    else if (ueL3->rrcData->state == UMTS_RRC_STATE_CONNECTED &&
            ueL3->rrcData->subState == UMTS_RRC_SUBSTATE_CELL_DCH)
    {
        // if any active cell RSCP is too low
        // or PriCell RSCP is not strong enough
        // or too long not to report
#if 0
        double priCellRscp =
            ueL3->priNodeB->cpichRscpMeas->GetAvgMeas(node->getNodeTime());

        if (bestRscpVal - ueL3->para.Qhyst > priCellRscp)
        {
            if (DEBUG_MEASUREMENT)
            {
                UmtsLayer3PrintRunTimeInfo(
                            node, UMTS_LAYER3_SUBLAYER_RRC);
                printf("\n\tPri cell %d 's RSCP is %f, the best RSCP "
                       " %f , Threhsold %f, need actively report "
                       "measurement to RNC\n",
                       ueL3->priNodeB->cellId, priCellRscp,
                       bestRscpVal, ueL3->para.Qhyst);
            }
            if (!needRepMeas)
            {
                needRepMeas = TRUE;
            }
        }
#endif

        if (needRepMeas ||
            (ueL3->lastReportTime +
            UMTS_DEFAULT_CELL_EVALUATION_TIME) < node->getNodeTime())
        {
            UmtsLayer3UeReportMeasurement(node, umtsL3, ueL3);
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3UeHandleCpichMeasuremnet
// LAYER      :: Layer3
// PURPOSE    :: Handle measurement msg.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data*   : Pointer to L3 data
// + ueL3             : UmtsLayer3Ue *    : Pointer to l3 Ue data
// + cmd              : void   *          : measurement report to interpret.
// RETURN     :: void : NULL
// **/
static
void  UmtsLayer3UeHandleMeasuremnetInd(
          Node* node,
          UmtsLayer3Data* umtsL3,
          UmtsLayer3Ue *ueL3,
          void* cmd)
{
    UmtsCphyMeasIndInfo* measInd;

    measInd = (UmtsCphyMeasIndInfo*) cmd;

    if (measInd->measType == UMTS_MEASURE_L1_CPICH_RSCP ||
        measInd->measType == UMTS_MEASURE_L1_CPICH_Ec_No)
    {
        UmtsLayer3UeHandleCpichMeasurement(node,umtsL3, ueL3, cmd);
    }
    else
    {
        // TODO
    }
}

// /**
// FUNCTION   :: UmtsLayer3UeCheckLinkFailure
// LAYER      :: Layer3
// PURPOSE    :: Check whether physical links are broken
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data*   : Pointer to L3 data
// + ueL3             : UmtsLayer3Ue *    : Pointer to l3 Ue data
// RETURN     :: void : NULL
// **/
static
void  UmtsLayer3UeCheckLinkFailure(
          Node* node,
          UmtsLayer3Data* umtsL3,
          UmtsLayer3Ue *ueL3)
{
    BOOL linkFail = TRUE;
    std::list<UmtsLayer3UeNodebInfo*>::iterator it;
    for (it = ueL3->ueNodeInfo->begin();
        it != ueL3->ueNodeInfo->end();
        it++)
    {
        if ((*it)->cellStatus == UMTS_CELL_ACTIVE_SET)
        {
            linkFail = FALSE;
            break;
        }
    }

    if (linkFail)
    {
        if (DEBUG_RR_RELEASE)
        {
            UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
            printf(
                "\n\tRelease RRC_CONNECTION_RELEASE due to link failure\n");
        }
        UmtsLayer3UeReleaseDedctRsrc(node, umtsL3);
        UmtsLayer3UeEnterIdle(node, umtsL3, ueL3);
        UmtsLayer3UeUpdateStatAfterFailure(node, umtsL3, ueL3);
        UmtsLayer3UeRrcConnRelInd(node, umtsL3);

        // do cell reselection
        UmtsLayer3UeSearchNewCell(node, umtsL3, ueL3);
    }

    if (ueL3->priNodeB == NULL && !ueL3->ueNodeInfo->empty())
    {
        ueL3->priNodeB = ueL3->ueNodeInfo->front();
    }
}

// /**
// FUNCTION   :: UmtsLayer3UeHandleTimerCpichCheck
// LAYER      :: Layer3
// PURPOSE    :: Handle TIMER CPICHCHECK
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data*   : Pointer to L3 data
// + ueL3             : UmtsLayer3Ue *    : Pointer to l3 Ue data
// + msg              : Message*          : Message to be handled
// RETURN     :: void : NULL
// **/
static
BOOL UmtsLayer3UeHandleTimerCpichCheck(
          Node* node,
          UmtsLayer3Data* umtsL3,
          UmtsLayer3Ue *ueL3,
          Message* msg)
{
    std::list<UmtsLayer3UeNodebInfo*>::iterator it;

    for (it = ueL3->ueNodeInfo->begin();
        it != ueL3->ueNodeInfo->end();
        it ++)
    {
        if ((*it)->cpichRscpMeas->lastTimeStamp
                + UMTS_CPICH_MEAS_OBSLT_TRH
                < node->getNodeTime())
        {
            if (ueL3->priNodeB == (*it))
            {
                ueL3->priNodeB = NULL;
            }
            delete (*it);
            (*it) = NULL;
        }
    }
    ueL3->ueNodeInfo->remove(NULL);

    if (ueL3->rrcData->state == UMTS_RRC_STATE_CONNECTED
        && ueL3->rrcData->subState == UMTS_RRC_SUBSTATE_CELL_DCH)
    {
        UmtsLayer3UeCheckLinkFailure(node, umtsL3, ueL3);
    }
    else if (ueL3->ueNodeInfo->empty())
    {
        ueL3->mmData->idleSubState = UMTS_MM_MS_IDLE_NO_CELL_AVAILABLE;
        ueL3->rrcData->subState = UMTS_RRC_SUBSTATE_NO_PLMN;
    }
    else if (ueL3->priNodeB == NULL)
    {
        ueL3->priNodeB = ueL3->ueNodeInfo->front();
    }

    UmtsLayer3SetTimer(node,
                       umtsL3,
                       UMTS_LAYER3_TIMER_CPICHCHECK,
                       UMTS_LAYER3_TIMER_CPICHCHECK_VAL,
                       msg,
                       NULL,
                       0);
    return TRUE;
}

// /**
// FUNCTION   :: UmtsLayer3UeHandleTimerT300
// LAYER      :: Layer 3
// PURPOSE    :: Handle a T300 timer expired msg
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + ueL3      : UmtsLayer3Ue *   : UE data
// + msg       : Message*         : Message containing the packet
// RETURN     :: BOOL : msg need to be reused or not
// **/
static
BOOL UmtsLayer3UeHandleTimerT300(Node *node,
                                 UmtsLayer3Data *umtsL3,
                                 UmtsLayer3Ue *ueL3,
                                 Message *msg)
{
    UmtsRrcUeData* ueRrc = ueL3->rrcData;
    UmtsLayer3CnDomainId domainIndi = (UmtsLayer3CnDomainId)
                                      (* (MESSAGE_ReturnInfo(msg)
                                          + sizeof(UmtsLayer3Timer)));
    ueRrc->timerT300 = NULL;
    UmtsLayer3UeSendRrcConnReq(node,
                               umtsL3,
                               domainIndi);

    // update stat
    ueL3->stat.numRrcConnReqSent ++;

    return FALSE;
}

// /**
// FUNCTION   :: UmtsLayer3UeHandleTimerT308
// LAYER      :: Layer 3
// PURPOSE    :: Handle a T308 timer expired msg
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + ueL3      : UmtsLayer3Ue *   : UE data
// + msg       : Message*         : Message containing the packet
// RETURN     :: BOOL : msg need to be reused or not
// **/
static
BOOL UmtsLayer3UeHandleTimerT308(Node *node,
                                 UmtsLayer3Data *umtsL3,
                                 UmtsLayer3Ue *ueL3,
                                 Message *msg)
{
    UmtsRrcUeData* ueRrc = ueL3->rrcData;
    ueRrc->timerT308 = NULL;
    BOOL msgReuse = FALSE;

    ueRrc->counterV308++;
    if (ueRrc->counterV308 > UMTS_LAYER3_TIMER_N308)
    {
        if (DEBUG_RR_RELEASE)
        {
            UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
            printf("\n\tHandle T308: Release "
                "RRC_CONNECTION_RELEASE v308 > N308\n");
        }
        UmtsLayer3UeReleaseDedctRsrc(node, umtsL3);
        UmtsLayer3UeEnterIdle(node, umtsL3, ueL3);
        UmtsLayer3UeRrcConnRelInd(node, umtsL3);

        // do cell reselection
        UmtsLayer3UeSearchNewCell(node, umtsL3, ueL3);
    }
    else
    {
        UmtsLayer3UeSendRrcConnRelComp(node, umtsL3);

        // update stat
        ueL3->stat.numRrcConnRelCompSent ++;
        ueRrc->timerT308 =
            UmtsLayer3SetTimer(node,
                               umtsL3,
                               UMTS_LAYER3_TIMER_T308,
                               UMTS_LAYER3_TIMER_T308_VAL,
                               msg,
                               NULL,
                               0);
        msgReuse = TRUE;
    }
    return msgReuse;
}

// /**
// FUNCTION   :: UmtsLayer3UeHandleTimerRrcConnReqWaitTime
// LAYER      :: Layer 3
// PURPOSE    :: Handle a RRC_CONNECTION_REQUEST waittime timer expired msg
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + ueL3      : UmtsLayer3Ue *   : UE data
// + msg       : Message*         : Message containing the packet
// RETURN     :: BOOL : msg need to be reused or not
// **/
static
BOOL UmtsLayer3UeHandleTimerRrcConnReqWaitTime(
        Node *node,
        UmtsLayer3Data *umtsL3,
        UmtsLayer3Ue *ueL3,
        Message *msg)
{
    UmtsRrcUeData* ueRrc = ueL3->rrcData;
    UmtsLayer3CnDomainId domainIndi = ueRrc->rrcConnReqInfo.cnDomainId;
    ueRrc->timerRrcConnReqWaitTime = NULL;
    UmtsLayer3UeSendRrcConnReq(node,
                            umtsL3,
                            domainIndi);
    // update stat
    ueL3->stat.numRrcConnReqSent ++;

    return FALSE;
}

//
// RRC interface for MM and GMM
//
// /**
// FUNCTION   :: UmtsLayer3UeInitRrcConnReq
// LAYER      :: Layer3 RRC
// PURPOSE    :: Initiate RRC connection request by MM/GMM
// PARAMETERS ::
// +node      : Node*               : Pointer to node.
// +umtsL3    : UmtsLayer3Data *    : Pointer to UMTS Layer3 data
// +cause     : UmtsRrcEstCause     : Establishment Cause
// +domainIndi: UmtsLayer3CnDomainId: Domain indicator
// +connected : BOOL                : For returning whether there
//                                    is RRC connection already
// RETURN     :: BOOL               : TRUE if RRC connection request
//                                    has been sent
// **/
static
BOOL UmtsLayer3UeInitRrcConnReq(
    Node* node,
    UmtsLayer3Data *umtsL3,
    UmtsRrcEstCause cause,
    UmtsLayer3CnDomainId domainIndi,
    BOOL* connected)
{
    UmtsLayer3Ue *ueL3 = (UmtsLayer3Ue *) (umtsL3->dataPtr);
    UmtsRrcUeData* ueRrc = ueL3->rrcData;

    *connected = FALSE;
    BOOL retVal = FALSE;
    if (ueRrc->state == UMTS_RRC_STATE_CONNECTED)
    {
        *connected = TRUE;
    }
    else if (ueRrc->subState == UMTS_RRC_SUBSTATE_CAMPED_NORMALLY)
    {
        if (ueRrc->timerT308)
        {
            // if rrc Rl comp has been sent, but T308 still running
            // cancel it,and start a newround of RRC request
            MESSAGE_CancelSelfMsg(node, ueRrc->timerT308);
            ueRrc->timerT308 = NULL;
            //Release Dedicated channel since we don't need to send
            //RRC connection release complete
            UmtsLayer3UeReleaseDedctRsrc(node, umtsL3);
        }
        ueRrc->estCause = cause;
        ueRrc->protocolErrorIndi = FALSE;
        ueRrc->rrcConnReqInfo.cnDomainId = domainIndi;
        UmtsLayer3UeSendRrcConnReq(node,
                                   umtsL3,
                                   domainIndi);

        // update stat
        ueL3->stat.numRrcConnReqSent ++;

        retVal = TRUE;
    }

    return retVal;
}

// FUNCTION   :: UmtsLayer3UeSendNasMsg
// LAYER      :: Layer3 RRC
// PURPOSE    :: Requested by upper layer to send NAS message.
//               Note: RRC connection must be established before this
//               function is called
// PARAMETERS ::
// + node      : Node*               : Pointer to node.
// + umtsL3    : UmtsLayer3Data *    : Pointer to UMTS Layer3 data
// + msg       : char*               : Pointer to the NAS message content
// + nasmsgSize   : int              : NAS message size
// + pd        : char                : Protocol discriminator
// + msgType   : char                : NAS message type
// + transctId : unsigned char                : Transaction ID
// + domain    : UmtsLayer3CnDomainId: Domain indicator
// RETURN     :: BOOL               : TRUE if the message has been sent
// **/
static
BOOL UmtsLayer3UeSendNasMsg(Node* node,
                            UmtsLayer3Data *umtsL3,
                            char* nasMsg,
                            int   nasMsgSize,
                            char  pd,
                            char  msgType,
                            unsigned char  transactId,
                            UmtsLayer3CnDomainId domain)
{
    UmtsLayer3Ue *ueL3 = (UmtsLayer3Ue*)(umtsL3->dataPtr);
    UmtsRrcUeData* ueRrc = ueL3->rrcData;
    if (ueRrc->state != UMTS_RRC_STATE_CONNECTED)
    {
        return FALSE;
    }

    UmtsLayer3Header l3Header;
    l3Header.tiSpd = transactId;
    l3Header.pd = pd;
    l3Header.msgType = msgType;

    int packetSize = nasMsgSize + sizeof(UmtsLayer3Header) + 1;
    Message* msg =
        MESSAGE_Alloc(node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        msg,
                        packetSize,
                        TRACE_UMTS_LAYER3);
    char* packet = MESSAGE_ReturnPacket(msg);
    (*packet) = (char)domain;
    memcpy(packet + 1, &l3Header, sizeof(UmtsLayer3Header));
    if (nasMsg && nasMsgSize > 0)
    {
        memcpy(packet + 1 + sizeof(UmtsLayer3Header), nasMsg, nasMsgSize);
    }

    UmtsRRMessageType rrcMsgType;
    //If there is no signalling connection for the requested domain,
    //use initial direct transfer to send the message
    if (!ueRrc->signalConn[domain])
    {
        ueRrc->signalConn[domain] = TRUE;
        rrcMsgType = UMTS_RR_MSG_TYPE_INITIAL_DIRECT_TRANSFER;
    }
    else
    {
        rrcMsgType = UMTS_RR_MSG_TYPE_UL_DIRECT_TRANSFER;
    }
    UmtsLayer3AddHeader(
        node,
        msg,
        UMTS_LAYER3_PD_RR,
        0,
        (char)rrcMsgType);

    //Use RB3 to send NAS message
    UmtsLayer3UeSendPacketToLayer2(
        node,
        umtsL3->interfaceIndex,
        UMTS_SRB3_ID,
        msg);

    // update stat
    ueL3->stat.numNasMsgSentToL2 ++;

    return TRUE;
}

//
// handle MM Msg
//
// /**
// FUNCTION   :: UmtsMmHandleTimerT3212
// LAYER      :: Layer 3
// PURPOSE    :: Handle a T3212 timer expired msg
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + ueL3      : UmtsLayer3Ue *   : UE data
// + msg       : Message*         : Message containing the packet
// RETURN     :: BOOL : msg need to be reused or not
// **/
static
BOOL UmtsMmHandleTimerT3212(Node *node,
                             UmtsLayer3Data *umtsL3,
                             UmtsLayer3Ue *ueL3,
                             Message *msg)
{
    // TODO: It is time to have periodic location update
    // if it is idle, loc up needed,
    // if it is connect skip it. after connect -> idle,
    // new cell will be selected, and after finish cell select
    // start a new T3212
    return FALSE;
}

// /**
// FUNCTION   :: UmtsMmHandleTimerT3230
// LAYER      :: Layer 3
// PURPOSE    :: Handle a T3230 timer expired msg
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + ueL3      : UmtsLayer3Ue *   : UE data
// + msg       : Message*         : Message containing the packet
// RETURN     :: NULL
// **/
static
void UmtsMmHandleTimerT3230(
        Node *node,
        UmtsLayer3Data *umtsL3,
        UmtsLayer3Ue *ueL3,
        Message *msg)
{
    UmtsMmMsData* ueMm = ueL3->mmData;
    if (!ueMm)
    {
        return;
    }

    if (ueMm->mainState == UMTS_MM_MS_WAIT_FOR_OUTGOING_MM_CONNECTION
        || ueMm->mainState ==
                UMTS_MM_MS_WAIT_FOR_ADDITIONAL_OUTGOING_MM_CONNECTION)
    {
        // remove queued CM service requests
        // Notify each affected CM entities
        while (!ueMm->rqtingMmConns->empty())
        {
            UmtsLayer3PD pd = ueMm->rqtingMmConns->front()->pd;
            unsigned char ti = ueMm->rqtingMmConns->front()->ti;
            delete ueMm->rqtingMmConns->front();
            ueMm->rqtingMmConns->pop_front();

            UmtsLayer3UeMmConnEstInd(node, umtsL3, pd, ti, FALSE);
        }

        // If no active MM conection,
        // Release CS domain signal connection
        if (ueMm->actMmConns->empty())
        {
            UmtsLayer3UeSendSignalConnRelIndi(
                node,
                umtsL3,
                ueL3,
                UMTS_LAYER3_CN_DOMAIN_CS);
            ueL3->stat.numSigConnRelIndSent ++;

            ueMm->mainState = UMTS_MM_MS_MM_IDLE;
            ueMm->idleSubState = UMTS_MM_MS_IDLE_NORMAL_SERVICE;
        }
    }
    ueMm->T3230 = NULL;
}

// /**
// FUNCTION   :: UmtsMmHandleTimerT3240
// LAYER      :: Layer 3
// PURPOSE    :: Handle a T3240 timer expired msg
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + ueL3      : UmtsLayer3Ue *   : UE data
// + msg       : Message*         : Message containing the packet
// RETURN     :: NULL
// **/
static
void UmtsMmHandleTimerT3240(
        Node *node,
        UmtsLayer3Data *umtsL3,
        UmtsLayer3Ue *ueL3,
        Message *msg)
{
    UmtsMmMsData* ueMm = ueL3->mmData;
    if (!ueMm)
    {
        return;
    }

    if (ueMm->mainState == UMTS_MM_MS_WAIT_FOR_NETWORK_COMMAND)
    {
        UmtsLayer3UeSendSignalConnRelIndi(
            node,
            umtsL3,
            ueL3,
            UMTS_LAYER3_CN_DOMAIN_CS);
        ueL3->stat.numSigConnRelIndSent ++;

        ueMm->mainState = UMTS_MM_MS_MM_IDLE;
        ueMm->idleSubState = UMTS_MM_MS_IDLE_NORMAL_SERVICE;
    }
    ueMm->T3240 = NULL;
}

// /**
// FUNCTION   :: UmtsLayer3UeHandleLocUpAcpt
// LAYER      :: Layer 3
// PURPOSE    :: Handle a Location update Accept message .
// PARAMETERS ::
// + node           : Node*            : Pointer to node.
// + umtsL3         : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + transactId     : unsigned char    : Transaction ID
// + msg            : char*  : Pointer to the NAS message contents
// + msgSize        : int              : Message size
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3UeHandleLocUpAcpt(
    Node*               node,
    UmtsLayer3Data*     umtsL3,
    unsigned char       transactId,
    char*               msg,
    int                 msgSize)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*) umtsL3->dataPtr;
    UmtsLayer3RoutingUpdateAccept raUpAcpt;
    memcpy(&raUpAcpt, msg, sizeof(raUpAcpt));

    if (DEBUG_MM)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_MM);
        printf("\n\treceives a location update accept message\n");
    }

    // update stat
    ueL3->stat.numLocUpAcptRcvd ++;

    // handle the contents in the accept msg
    unsigned char upInterval;
    unsigned char upUnit;
    BOOL schedRaUpTimer = TRUE;
    upInterval = raUpAcpt.periodicRaUpTime & 0x1F;
    upUnit = (raUpAcpt.periodicRaUpTime >> 5) & 0x08;
    ueL3->locRoutAreaId = raUpAcpt.rncId;

    if (upUnit == 0)
    {
        ueL3->mmData->periodicUpInterval = upInterval * 2 * SECOND;
    }
    else if (upUnit == 2)
    {
        ueL3->mmData->periodicUpInterval = upInterval * 6 * MINUTE;
    }
    else if (upUnit == 7)
    {
       schedRaUpTimer = FALSE;
    }
    else
    {
        ueL3->mmData->periodicUpInterval = upInterval * 1 * MINUTE;
    }

#if 0
    if (ueL3->mmData->T3212)
    {
        MESSAGE_CancelSelfMsg(node, ueL3->mmData->T3212);
        ueL3->mmData->T3212 = NULL;
    }
#endif // 0
    if (schedRaUpTimer)
    {
        // schedule perioidc loction update
        /*ueL3->mmData->T3211 =
            UmtsLayer3SetTimer(
                node,
                umtsL3,
                UMTS_LAYER3_TIMER_T3212,
                ueL3->mmData->periodicUpInterval,
                NULL);*/
    }

    ueL3->mmData->mainState = UMTS_MM_MS_WAIT_FOR_NETWORK_COMMAND;
    ueL3->mmData->updateState = UMTS_MM_UPDATED;

    // by assuming Loc area and routing are idential
    // then GGM updetd as well
    ueL3->gmmData->updateState = UMTS_GMM_UPDATED;

    //Release signal connection
    //TODO: check if there is FLOW ON PROCEED
    if (ueL3->gmmData->mainState == UMTS_GMM_MS_DEREGISTERED &&
        ueL3->gmmData->deRegSubState ==
            UMTS_GMM_MS_DEREGISTERED_ATTACH_NEEDED)
    {
        UmtsLayer3UeInitGmmAttachProc(
            node, umtsL3, ueL3, FALSE, FALSE);
    }

    //Initiate T3240 to release RR connection (CS signalling connection)
    ERROR_Assert(ueL3->mmData->T3240 == NULL, "Inconsistent MM state");
    ueL3->mmData->T3240 = UmtsLayer3SetTimer(
                    node,
                    umtsL3,
                    UMTS_LAYER3_TIMER_T3240,
                    UMTS_LAYER3_TIMER_T3240_VAL,
                    NULL,
                    NULL,
                    0);
}

// /**
// FUNCTION   :: UmtsLayer3UeHandleLocUpRej
// LAYER      :: Layer 3
// PURPOSE    :: Handle a Location update Reject message.
// PARAMETERS ::
// + node           : Node*            : Pointer to node.
// + umtsL3         : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + transactId     : char             : Transaction ID
// + msg            : char*            : Pointer to the message contents
// + msgSize        : int              : Message size
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3UeHandleLocUpRej(
    Node*               node,
    UmtsLayer3Data*     umtsL3,
    unsigned char       transactId,
    char*               msg,
    int                 msgSize)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*) umtsL3->dataPtr;

    // update stat
    ueL3->stat.numLocUpRejRcvd ++;
    if (DEBUG_MM)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_MM);
        printf("\n\tReceives a location update reject message\n");
    }
}

// /**
// FUNCTION   :: UmtsLayer3UeHandleCmServAcpt
// LAYER      :: Layer 3
// PURPOSE    :: Handle a CM Service Acpt message.
// PARAMETERS ::
// + node           : Node*            : Pointer to node.
// + umtsL3         : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + transactId     : char             : Transaction ID
// + msg            : char*            : Pointer to the CM message contents
// + msgSize        : int              : Message size
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3UeHandleCmServAcpt(
    Node*               node,
    UmtsLayer3Data*     umtsL3,
    unsigned char       ti,
    char*               msg,
    int                 msgSize)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*) umtsL3->dataPtr;
    UmtsMmMsData* ueMm = ueL3->mmData;

    UmtsLayer3PD pd;
    memcpy(&pd, msg, sizeof(pd));

    if (ueMm->mainState == UMTS_MM_MS_WAIT_FOR_OUTGOING_MM_CONNECTION ||
        ueMm->mainState ==
            UMTS_MM_MS_WAIT_FOR_ADDITIONAL_OUTGOING_MM_CONNECTION)
    {
        UmtsLayer3MmConn* mmConn = ueMm->rqtingMmConns->front();
        if (mmConn && mmConn->pd == pd && mmConn->ti == ti)
        {
            ERROR_Assert(ueMm->T3230, "Inconsistent MM state");
            MESSAGE_CancelSelfMsg(node, ueMm->T3230);
            ueMm->T3230 = NULL;

            ueMm->actMmConns->push_back(mmConn);
            ueMm->rqtingMmConns->pop_front();
            ueMm->mainState = UMTS_MM_MS_MM_CONNECTION_ACTIVE;
            UmtsLayer3UeMmConnEstInd(node, umtsL3, pd, ti, TRUE);

            //check if there is pending MM conn request
            if (!ueMm->rqtingMmConns->empty())
            {
                UmtsLayer3UeSendCmServReq(
                    node,
                    umtsL3,
                    ueMm,
                    ueMm->rqtingMmConns->front()->pd,
                    ueMm->rqtingMmConns->front()->ti);
                ueMm->mainState =
                    UMTS_MM_MS_WAIT_FOR_ADDITIONAL_OUTGOING_MM_CONNECTION;
            }
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3UeHandleMmMsg
// LAYER      :: Layer 3
// PURPOSE    :: Handle a Mobility Management message from the network .
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msgType        : UmtsMmMessageType      : Message type
// + transactId     : char                   : Transaction ID
// + msg            : char*  : Pointer to the NAS message contents
// + msgSize        : int                    : Message size
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3UeHandleMmMsg(
    Node*               node,
    UmtsLayer3Data*     umtsL3,
    UmtsMmMessageType   msgType,
    unsigned char       transactId,
    char*               msg,
    int                 msgSize)
{
    switch (msgType)
    {
        case UMTS_MM_MSG_TYPE_LOCATION_UPDATING_ACCEPT:
        {
            UmtsLayer3UeHandleLocUpAcpt(
                node,
                umtsL3,
                transactId,
                msg,
                msgSize);
            break;
        }
        case UMTS_MM_MSG_TYPE_LOCATION_UPDATING_REJECT:
        {
            UmtsLayer3UeHandleLocUpRej(
                node,
                umtsL3,
                transactId,
                msg,
                msgSize);
            break;
        }
        case UMTS_MM_MSG_TYPE_CM_SERVICE_ACCEPT:
        {
            UmtsLayer3UeHandleCmServAcpt(
                node,
                umtsL3,
                transactId,
                msg,
                msgSize);
            break;
        }
        default:
        {
            char errStr[MAX_STRING_LENGTH];
            sprintf(errStr,
                    "UMTS Layer3: UE: %d receivers a MM "
                    "signalling msg with unknown type: %d",
                    node->nodeId,
                    msgType);
            ERROR_ReportWarning(errStr);
        }
    }
}

//
// handle GMM Msg
//

// /**
// FUNCTION   :: UmtsGMmUeUpdateGMmState
// LAYER      :: Layer3 GMM
// PURPOSE    :: Handle GMM state update
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data*   : Pointer to L3 data
// + ueL3             : UmtsLayer3Ue *    : Pointer to l3 Ue data
// + eventType       : UmtsGMmStateUpdateEventType : update event
// RETURN     :: void : NULL
// **/
static
void UmtsGMmUeUpdateGMmState(
         Node* node,
         UmtsLayer3Data* umtsL3,
         UmtsLayer3Ue* ueL3,
         UmtsGMmStateUpdateEventType eventType)
{
    if (UmtsGMmUeGetGMmState(ueL3) == UMTS_GMM_MS_NULL &&
        eventType == UMTS_GMM_UPDATE_EnableGprsMode)
    {
        UmtsGMmUeSetGMmState(ueL3, UMTS_GMM_MS_DEREGISTERED);

        // attach needed assume attach when poweron/init phase
        // the sirst attach will triggered when loction update is send
        ueL3->gmmData->deRegSubState =
            UMTS_GMM_MS_DEREGISTERED_ATTACH_NEEDED;
        ueL3->gmmData->nextAttachType = UMTS_GPRS_ATTACH_ONLY_NO_FOLLOW_ON;
        ueL3->gmmData->attachAtmpCount = 0;
    }
    else if (UmtsGMmUeGetGMmState(ueL3) == UMTS_GMM_MS_DEREGISTERED &&
        eventType == UMTS_GMM_UPDATE_DisableGprsMode)
    {
        UmtsGMmUeSetGMmState(ueL3, UMTS_GMM_MS_NULL);
    }
    else if (UmtsGMmUeGetGMmState(ueL3) == UMTS_GMM_MS_DEREGISTERED &&
        eventType == UMTS_GMM_UPDATE_AttachRequested)
    {
        UmtsGMmUeSetGMmState(ueL3, UMTS_GMM_MS_REGISTERED_INITIATED);
    }
    else if (UmtsGMmUeGetGMmState(ueL3) ==
        UMTS_GMM_MS_REGISTERED_INITIATED &&
        eventType == UMTS_GMM_UPDATE_AttachAccepted)
    {
        UmtsGMmUeSetGMmState(ueL3, UMTS_GMM_MS_REGISTERED);
    }
    else if (UmtsGMmUeGetGMmState(ueL3) ==
        UMTS_GMM_MS_REGISTERED_INITIATED &&
        eventType == UMTS_GMM_UPDATE_LowerLayerFailure)
    {
        UmtsGMmUeSetGMmState(ueL3, UMTS_GMM_MS_DEREGISTERED);
    }
    else if (UmtsGMmUeGetGMmState(ueL3) ==
        UMTS_GMM_MS_REGISTERED_INITIATED &&
        eventType == UMTS_GMM_UPDATE_DetachRequestedNotPowerOff)
    {
        UmtsGMmUeSetGMmState(ueL3, UMTS_GMM_MS_DEREGISTERED_INITIATED);
    }
    else if (eventType == UMTS_GMM_UPDATE_DetachRequestedPowerOff ||
        (UmtsGMmUeGetGMmState(ueL3) ==
        UMTS_GMM_MS_DEREGISTERED_INITIATED &&
        eventType == UMTS_GMM_UPDATE_DetachAccepted))
    {
        UmtsGMmUeSetGMmState(ueL3, UMTS_GMM_MS_DEREGISTERED);
    }
    else
    {
        ERROR_ReportWarning("Unsupport GMM  update event");
    }

    // by assuming location area and routing area are the same
    // routing update is considered as the same as location update
}

// /**
// FUNCTION   :: UmtsGMmHandleTimerT3310
// LAYER      :: Layer 3
// PURPOSE    :: Handle a T3310 timer expired msg
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + ueL3      : UmtsLayer3Ue *   : UE data
// + msg       : Message*         : Message containing the packet
// RETURN     :: BOOL : msg need to be reused or not
// **/
static
BOOL UmtsGMmHandleTimerT3310(Node *node,
                             UmtsLayer3Data *umtsL3,
                             UmtsLayer3Ue *ueL3,
                             Message *msg)
{
    UmtsGMmMsData* ueGmm = ueL3->gmmData;

    UmtsAttachType* attachType;

    attachType = (UmtsAttachType*) MESSAGE_ReturnInfo(msg);

    // TODO
    // to see if attach attemp
    // get the attach type from the timer info
    // send the attach request again
    ueGmm->attachAtmpCount ++;
    if (ueGmm->attachAtmpCount <= UMTS_LAYER3_TIMER_GMM_MAXCOUNTER)
    {
        // send the attach request
        UmtsGMmUeSendAttachRequest(node, umtsL3, *attachType);
        ueL3->stat.numAttachReqSent ++;

        ueL3->gmmData->T3310 =
            UmtsLayer3SetTimer(
                node, umtsL3, UMTS_LAYER3_TIMER_T3310,
                UMTS_LAYER3_TIMER_T3310_VAL,
                NULL,
                (void*)&attachType, sizeof(UmtsAttachType));
    }
    else
    {
        ueL3->gmmData->T3310 = NULL;
        // notify applicaiton if any
        UmtsGMmUeUpdateGMmState(
            node, umtsL3, ueL3, UMTS_GMM_UPDATE_LowerLayerFailure);
    }

    return FALSE;
}

// /**
// FUNCTION   :: UmtsLayer3UeHandleAttachAcpt
// LAYER      :: Layer 3
// PURPOSE    :: Handle a Location update Accept message .
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + transactId     : char                   : Transaction ID
// + msg            : char* : Pointer to the NAS message contents
// + msgSize        : int                    : Message size
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3UeHandleAttachAcpt(
    Node*               node,
    UmtsLayer3Data*     umtsL3,
    unsigned char       transactId,
    char*               msg,
    int                 msgSize)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*) umtsL3->dataPtr;
    if (DEBUG_GMM)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_GMM);
        printf("\n\t Receives a attach accept message\n");
    }

    // update stat
    ueL3->stat.numAttachAcptRcvd ++;

    // TODO: store the received routing area code
    //
    // get teh TMSI allocated
    memcpy(&(ueL3->tmsi), &msg[0], sizeof(UInt32));

    // stop T3310
    if (ueL3->gmmData->T3310)
    {
        MESSAGE_CancelSelfMsg(node, ueL3->gmmData->T3310);
        ueL3->gmmData->T3310 = NULL;
    }
    // clear the attemp count
    ueL3->gmmData->attachAtmpCount = 0;
    ueL3->gmmData->raUpdAtmpCount = 0;

    if (UmtsGMmUeGetGMmState(ueL3) == UMTS_GMM_MS_REGISTERED_INITIATED)
    {
        UmtsGMmUeSetGMmState(ueL3, UMTS_GMM_MS_REGISTERED);
    }
    else
    {
        //TODO: handling other cases
    }

    // update the regited substate
    // update the updated state
    ueL3->gmmData->updateState = UMTS_GMM_UPDATED;
    ueL3->gmmData->pmmState = UMTS_PMM_CONNECTED;

    // TODO: assume PS signalling not release right now
    //ueL3->gmmData->psSignConnEsted = TRUE;

    // only when RRC disconnected, it will move to idle

    //Check if there is pending application
    BOOL followOnProc = FALSE;
    UmtsLayer3UeAppInfo* appInfo = NULL;
    std::list<UmtsLayer3UeAppInfo*>::iterator it;
    for (it = ueL3->appInfoList->begin();
        it != ueL3->appInfoList->end();
        it ++)
    {
        if ((*it)->classifierInfo->domainInfo == UMTS_LAYER3_CN_DOMAIN_PS &&
            (*it)->srcDestType == UMTS_FLOW_MOBILE_ORIGINATED)
        {
            followOnProc = TRUE;
            appInfo = (*it);
            break;
        }
    }

    // send ATTACH COMPLETE
    UmtsGMmUeSendAttachCompl(node, umtsL3, followOnProc);
    ueL3->stat.numAttachComplSent ++;

    if (followOnProc)
    {
        unsigned char serviceType;
        if (appInfo->cmData.smData->smState ==
            UMTS_SM_MS_PDP_INACTIVE)
        {
            serviceType = 0x00;
            appInfo->cmData.smData->needSendActivateReq = TRUE;
        }
        else
        {
            serviceType = 0x01;
        }
        unsigned char transId = appInfo->transId;
        UmtsLayer3UeInitServiceRequest(
           node, umtsL3, ueL3,
           UMTS_RRC_ESTCAUSE_ORIGINATING_BACKGROUND,
           serviceType, transId);
    }
    else
    {
        ueL3->gmmData->AttCompWaitRelInd = TRUE;
    }
}

// /**
// FUNCTION   :: UmtsLayer3UeHandleAttachRej
// LAYER      :: Layer 3
// PURPOSE    :: Handle a Location update Reject message.
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + transactId     : char                   : Transaction ID
// + msg            : char* : Pointer to the NAS message contents
// + msgSize        : int                    : Message size
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3UeHandleAttachRej(
    Node*               node,
    UmtsLayer3Data*     umtsL3,
    unsigned char                transactId,
    char*               msg,
    int                 msgSize)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*) umtsL3->dataPtr;

    // update stat
    ueL3->stat.numAttachRejRcvd ++;
    if (DEBUG_GMM)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_GMM);
        printf("\n\t receives a attach reject message\n");
    }

}
// /**
// FUNCTION   :: UmtsLayer3UeHandleServiceAcpt
// LAYER      :: Layer 3
// PURPOSE    :: Handle a service Accept message .
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + transactId     : char                   : Transaction ID
// + msg            : char* : Pointer to the NAS message contents
// + msgSize        : int                    : Message size
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3UeHandleServiceAcpt(
    Node*               node,
    UmtsLayer3Data*     umtsL3,
    unsigned char       transactId,
    char*               msg,
    int                 msgSize)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*) umtsL3->dataPtr;
    if (DEBUG_GMM)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_GMM);
        printf("receives a service accept message\n");
    }

    // update stat
    ueL3->stat.numServiceAcptRcvd ++;

    // stop T3317
    if (ueL3->gmmData->T3317)
    {
        MESSAGE_CancelSelfMsg(node, ueL3->gmmData->T3317);
        ueL3->gmmData->T3317 = NULL;
    }

    // update GMM state
    ueL3->gmmData->mainState = UMTS_GMM_MS_REGISTERED;
    ueL3->gmmData->regSubState = UMTS_GMM_MS_REGISTERED_NORMAL_SERVICE;

    ueL3->gmmData->servReqWaitForRrc = FALSE;

    // move state to PMM-CONNECTED if now it is IDLE
    if (ueL3->gmmData->pmmState != UMTS_PMM_CONNECTED)
    {
        ueL3->gmmData->pmmState = UMTS_PMM_CONNECTED;
    }

    if (ueL3->gmmData->serviceTypeRequested == 0 ||
        ueL3->gmmData->serviceTypeRequested == 1) // 001 data
    {
        // TODO: start T3319
#if 0
        if (ueL3->gmmData->T3319 != NULL)
        {
            MESSAGE_CancelSelfMsg(node, ueL3->gmmData->T3319);
            ueL3->gmmData->T3319 = NULL;
        }

        ueL3->gmmData->T3319 =
                UmtsLayer3SetTimer(
                    node, umtsL3, UMTS_LAYER3_TIMER_T3319,
                    UMTS_LAYER3_TIMER_T3319_VAL, NULL,
                    NULL, 0);
#endif // 0
    }
    // since PS signalling has been connected, check all the active MO app
    // if they have packets to send
    // check if inactive app if actviate PDP req needs to be sent
    std::list<UmtsLayer3UeAppInfo*>::iterator it;

    for (it = ueL3->appInfoList->begin();
        it != ueL3->appInfoList->end();
        it ++)
    {
        if ((*it)->classifierInfo->domainInfo == UMTS_LAYER3_CN_DOMAIN_PS)
        {
            if ((*it)->cmData.smData->smState == UMTS_SM_MS_PDP_ACTIVE &&
                 (*it)->srcDestType == UMTS_FLOW_MOBILE_ORIGINATED)
            {
                Message* msgInBuffer = NULL;

                while (!(*it)->inPacketBuf->empty())
                {
                    msgInBuffer = (*it)->inPacketBuf->front();
                    (*it)->inPacketBuf->pop();
                    ueL3->stat.numPsDataPacketFromUpperDeQueue ++;
                    UmtsLayer3UeSendPacketToLayer2(node,
                                               umtsL3->interfaceIndex,
                                               (*it)->rbId,
                                               msgInBuffer);

                    // update stat
                    ueL3->stat.numPsDataPacketSentToL2 ++;
                }
                (*it)->pktBufSize = 0;
            }
            else if ((*it)->cmData.smData->needSendActivateReq)
            {
                // send PDP for this transId app only
                // here treat all the apps have differnt PDPD address & APN
                // even though some of them share the same PDP address & APN
                // so that the secondary activation PDP request procedure
                // is ignored
                UmtsLayer3UeSmInitActivatePDPContextRequest(
                    node,
                    umtsL3,
                    ueL3,
                    (*it));
            }
        }
    }
}

// /**
// FUNCTION   :: UmtsGMmHandleTimerT3317
// LAYER      :: Layer 3
// PURPOSE    :: Handle a T3317 timer expired msg
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + ueL3      : UmtsLayer3Ue *   : UE data
// + msg       : Message*         : Message containing the packet
// RETURN     :: BOOL : msg need to be reused or not
// **/
static
BOOL UmtsGMmHandleTimerT3317(Node *node,
                             UmtsLayer3Data *umtsL3,
                             UmtsLayer3Ue *ueL3,
                             Message *msg)
{
    UmtsGMmMsData* ueGmm = ueL3->gmmData;

    // Abort the Service Request Procedure
    if (ueGmm->mainState == UMTS_GMM_MS_SERVICE_REQUEST_INITIATED)
    {
        UmtsLayer3UeAbortServiceRequest(node, umtsL3, ueL3);
    }

    ueL3->gmmData->T3317 = NULL;
    return FALSE;
}

// /**
// FUNCTION   :: UmtsLayer3UeHandleGMmMsg
// LAYER      :: Layer 3
// PURPOSE    :: Handle a GPRS Mobility Management message from the network .
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msgType        : UmtsGMmMessageType      : Message type
// + transactId     : char                   : Transaction ID
// + msg            : char*   : Pointer to the NAS message contents
// + msgSize        : int                    : Message size
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3UeHandleGMmMsg(
    Node*               node,
    UmtsLayer3Data*     umtsL3,
    UmtsGMmMessageType  msgType,
    unsigned char        transactId,
    char*               msg,
    int                 msgSize)
{
    switch (msgType)
    {
        case UMTS_GMM_MSG_TYPE_ATTACH_ACCEPT:
        {
            UmtsLayer3UeHandleAttachAcpt(
                node,
                umtsL3,
                transactId,
                msg,
                msgSize);
            break;
        }
        case UMTS_GMM_MSG_TYPE_ATTACH_REJECT:
        {
            UmtsLayer3UeHandleAttachRej(
                node,
                umtsL3,
                transactId,
                msg,
                msgSize);
            break;
        }
        case UMTS_GMM_MSG_TYPE_SERVICE_ACCEPT:
        {
            UmtsLayer3UeHandleServiceAcpt(
                node,
                umtsL3,
                transactId,
                msg,
                msgSize);
            break;
        }
        default:
        {
            char errStr[MAX_STRING_LENGTH];
            sprintf(errStr,
                    "UMTS Layer3: UE: %d receivers a GMM "
                    "signalling msg with unknown type: %d",
                    node->nodeId,
                    msgType);
            ERROR_ReportWarning(errStr);
        }
    }
}

//
// handle CC Msg
//
// /**
// FUNCTION   :: UmtsLayer3UeHandleCcT303
// LAYER      :: Layer 3
// PURPOSE    :: Handle a T303 CC timer
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + ueL3           : UmtsLayer3Ue *         : Pointer to UMTS Layer3 data
// + msg            : char*  : Pointer to the NAS message contents
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3UeHandleCcT303(Node* node,
                              UmtsLayer3Data* umtsL3,
                              UmtsLayer3Ue* ueL3,
                              Message* msg)
{
    char* timerInfo = MESSAGE_ReturnInfo(msg);

    char ti;
    memcpy(&ti, timerInfo+sizeof(UmtsLayer3Timer), sizeof(ti));

    UmtsLayer3UeAppInfo* ueFlow = UmtsLayer3UeFindAppInfoFromList(
                                    node,
                                    ueL3,
                                    UMTS_FLOW_MOBILE_ORIGINATED,
                                    ti);
    if (ueFlow)
    {
        UmtsCcMsData* ueCc = ueFlow->cmData.ccData;
        ueCc->T303 = NULL;
        if (ueCc->state == UMTS_CC_MS_MM_CONNECTION_PENDING
            || ueCc->state == UMTS_CC_MS_CALL_INITIATED)
        {
            UmtsLayer3UeNotifyAppCallReject(
                node,
                umtsL3,
                ueFlow,
                APP_UMTS_CALL_REJECT_CAUSE_USER_UNREACHABLE);
            if (ueCc->state == UMTS_CC_MS_CALL_INITIATED)
            {
                //start call clearing procedure
                UmtsLayer3UeInitCallClearing(node,
                                             umtsL3,
                                             ueFlow,
                                             UMTS_CALL_CLEAR_DROP);
            }
            else
            {
                ueCc->state = UMTS_CC_MS_NULL;
                UmtsLayer3UeRemoveAppInfoFromList(
                    node,
                    ueL3,
                    ueFlow->srcDestType,
                    ueFlow->transId);
            }
        }
    }
}

// FUNCTION   :: UmtsLayer3UeHandleCcT310
// LAYER      :: Layer 3
// PURPOSE    :: Handle a T310 CC timer
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + ueL3           : UmtsLayer3Ue *         : Pointer to UMTS Layer3 data
// + msg            : char*  : Pointer to the NAS message contents
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3UeHandleCcT310(Node* node,
                              UmtsLayer3Data* umtsL3,
                              UmtsLayer3Ue* ueL3,
                              Message* msg)
{
    char* timerInfo = MESSAGE_ReturnInfo(msg);

    char ti;
    memcpy(&ti, timerInfo+sizeof(UmtsLayer3Timer), sizeof(ti));

    UmtsLayer3UeAppInfo* ueFlow = UmtsLayer3UeFindAppInfoFromList(
                                    node,
                                    ueL3,
                                    UMTS_FLOW_MOBILE_ORIGINATED,
                                    ti);
    if (ueFlow)
    {
        UmtsCcMsData* ueCc = ueFlow->cmData.ccData;
        ueCc->T310 = NULL;
        if (ueCc->state == UMTS_CC_MS_MOBILE_ORIGINATING_CALL_PROCEEDING)
        {
            //start call clearing procedure
            UmtsLayer3UeInitCallClearing(node,
                                         umtsL3,
                                         ueFlow,
                                         UMTS_CALL_CLEAR_DROP);
            UmtsLayer3UeNotifyAppCallReject(
                node,
                umtsL3,
                ueFlow,
                APP_UMTS_CALL_REJECT_CAUSE_USER_BUSY);
        }
    }
}

// FUNCTION   :: UmtsLayer3UeHandleCcT313
// LAYER      :: Layer 3
// PURPOSE    :: Handle a T313 CC timer
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + ueL3           : UmtsLayer3Ue *         : Pointer to UMTS Layer3 data
// + msg            : char*  : Pointer to the NAS message contents
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3UeHandleCcT313(Node* node,
                              UmtsLayer3Data* umtsL3,
                              UmtsLayer3Ue* ueL3,
                              Message* msg)
{
    char* timerInfo = MESSAGE_ReturnInfo(msg);

    char ti;
    memcpy(&ti, timerInfo+sizeof(UmtsLayer3Timer), sizeof(ti));

    UmtsLayer3UeAppInfo* ueFlow = UmtsLayer3UeFindAppInfoFromList(
                                    node,
                                    ueL3,
                                    UMTS_FLOW_NETWORK_ORIGINATED,
                                    ti);
    if (ueFlow)
    {
        //UmtsCcMsData* ueCc = ueFlow->cmData.ccData;
        ueFlow->cmData.ccData->T313 = NULL;
        if (ueFlow->cmData.ccData->state
                      == UMTS_CC_MS_CONNECT_REQUEST)
        {
            //start call clearing,
            UmtsLayer3UeInitCallClearing(node,
                                         umtsL3,
                                         ueFlow,
                                         UMTS_CALL_CLEAR_DROP);

            // send a MSG_APP_CELLULAR_FromNetworkCallRejected to the
            // Umtscall application.
            UmtsLayer3UeNotifyAppCallReject(
                node,
                umtsL3,
                ueFlow,
                APP_UMTS_CALL_REJECT_CAUSE_SYSTEM_BUSY);
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3UeHandleCcT305
// LAYER      :: Layer 3
// PURPOSE    :: Handle a T305 CC timer
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + ueL3           : UmtsLayer3Ue *         : Pointer to UMTS Layer3 data
// + msg            : char*   : Pointer to the NAS message contents
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3UeHandleCcT305(Node* node,
                              UmtsLayer3Data* umtsL3,
                              UmtsLayer3Ue* ueL3,
                              Message* msg)
{
    char* timerInfo = MESSAGE_ReturnInfo(msg);

    char ti;
    memcpy(&ti, timerInfo+sizeof(UmtsLayer3Timer), sizeof(ti));
    UmtsLayer3UeAppInfo* ueFlow = UmtsLayer3UeFindAppInfoFromList(
                                    node,
                                    ueL3,
                                    UMTS_FLOW_MOBILE_ORIGINATED,
                                    ti);
    if (ueFlow)
    {
        UmtsCcMsData* ueCc = ueFlow->cmData.ccData;
        ueCc->T305 = NULL;
        if (ueCc->state == UMTS_CC_MS_DISCONNECT_REQUEST)
        {
            UmtsLayer3UeSendCallRelease(node, umtsL3, ueFlow);
            ueCc->T308 = UmtsLayer3SetTimer(
                                node,
                                umtsL3,
                                UMTS_LAYER3_TIMER_CC_T308,
                                UMTS_LAYER3_TIMER_CC_T308_MS_VAL,
                                NULL,
                                (void*)&(ueFlow->transId),
                                sizeof(ueFlow->transId));
            ueCc->state = UMTS_CC_MS_RELEASE_REQUEST;
            ueCc->counterT308 = 0;
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3UeHandleCcT308
// LAYER      :: Layer 3
// PURPOSE    :: Handle a T308 CC timer
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + ueL3           : UmtsLayer3Ue *         : Pointer to UMTS Layer3 data
// + msg            : char* : Pointer to the NAS message contents
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3UeHandleCcT308(Node* node,
                              UmtsLayer3Data* umtsL3,
                              UmtsLayer3Ue* ueL3,
                              Message* msg)
{
    char* timerInfo = MESSAGE_ReturnInfo(msg);

    char ti;
    memcpy(&ti, timerInfo+sizeof(UmtsLayer3Timer), sizeof(ti));
    UmtsLayer3UeAppInfo* ueFlow = UmtsLayer3UeFindAppInfoFromList(
                                    node,
                                    ueL3,
                                    UMTS_FLOW_MOBILE_ORIGINATED,
                                    ti);
    if (ueFlow)
    {
        UmtsCcMsData* ueCc = ueFlow->cmData.ccData;
        ueCc->T308 = NULL;
        if (ueCc->state == UMTS_CC_MS_RELEASE_REQUEST)
        {
            ueCc->counterT308++;
            if (ueCc->counterT308 < 2)
            {
                UmtsLayer3UeSendCallRelease(node, umtsL3, ueFlow);
                ueCc->T308 = UmtsLayer3SetTimer(
                                    node,
                                    umtsL3,
                                    UMTS_LAYER3_TIMER_CC_T308,
                                    UMTS_LAYER3_TIMER_CC_T308_MS_VAL,
                                    NULL,
                                    (void*)&(ueFlow->transId),
                                    sizeof(ueFlow->transId));
            }
            else
            {
                ueCc->state = UMTS_CC_MS_NULL;
                // TODO: notify UmtsCall the call is ended

                char ti = ueFlow->transId;
                //remove the the application flow
                UmtsLayer3UeRemoveAppInfoFromList(
                    node,
                    ueL3,
                    ueFlow->srcDestType,
                    ueFlow->transId);
                //Remove this MM connection
                UmtsLayer3UeReleaseMmConn(
                    node,
                    umtsL3,
                    ueL3,
                    UMTS_LAYER3_PD_CC,
                    ti);
            }
        }
    }
}

// FUNCTION   :: UmtsLayer3UeHandleCallProc
// LAYER      :: Layer 3
// PURPOSE    :: Handle CALL PROCEEDING MESSAGE
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + ueL3           : UmtsLayer3Ue *         : Pointer to UMTS Layer3 data
// + msg            : char* : Pointer to the NAS message contents
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3UeHandleCallProc(
        Node* node,
        UmtsLayer3Data* umtsL3,
        UmtsLayer3UeAppInfo* ueFlow,
        char* msg,
        int msgSize)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*)(umtsL3->dataPtr);
    ueL3->stat.numCcCallProcRcvd++;
    if (DEBUG_CC)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_CC);
        printf("\n\t UE: %d rcvd an CALL PROCEED from the MSC\n",
                node->nodeId);
    }
    if (ueFlow)
    {
        UmtsCcMsData* ueCc = ueFlow->cmData.ccData;
        if (ueCc->state == UMTS_CC_MS_CALL_INITIATED)
        {
            ueCc->state = UMTS_CC_MS_MOBILE_ORIGINATING_CALL_PROCEEDING;
            MESSAGE_CancelSelfMsg(node, ueCc->T303);
            ueCc->T303 = NULL;
            ueCc->T310 = UmtsLayer3SetTimer(
                                node,
                                umtsL3,
                                UMTS_LAYER3_TIMER_CC_T310,
                                UMTS_LAYER3_TIMER_CC_T310_MS_VAL,
                                NULL,
                                (void*)&(ueFlow->transId),
                                sizeof(ueFlow->transId));
        }
    }
}

// FUNCTION   :: UmtsLayer3UeHandleCallAlerting
// LAYER      :: Layer 3
// PURPOSE    :: Handle a CC Alerting MESSAGE
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + ueL3           : UmtsLayer3Ue *         : Pointer to UMTS Layer3 data
// + msg            : char* : Pointer to the NAS message contents
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3UeHandleCallAlerting(
        Node* node,
        UmtsLayer3Data* umtsL3,
        UmtsLayer3UeAppInfo* ueFlow,
        char* msg,
        int msgSize)
{
    if (DEBUG_CC)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_CC);
        printf("\n\t UE: %d rcvd an ALERTING from the MSC\n",
                node->nodeId);
    }
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*)(umtsL3->dataPtr);
    ueL3->stat.numCcAlertingRcvd++;

    if (ueFlow)
    {
        UmtsCcMsData* ueCc = ueFlow->cmData.ccData;
        if (ueCc->state == UMTS_CC_MS_CALL_INITIATED)
        {
            ueCc->state = UMTS_CC_MS_CALL_DELIVERED;
            MESSAGE_CancelSelfMsg(node, ueCc->T303);
            ueCc->T303 = NULL;
        }
        else if (ueCc->state ==
            UMTS_CC_MS_MOBILE_ORIGINATING_CALL_PROCEEDING)
        {
            ueCc->state = UMTS_CC_MS_CALL_DELIVERED;
            MESSAGE_CancelSelfMsg(node, ueCc->T310);
            ueCc->T310 = NULL;
        }
    }
}

// FUNCTION   :: UmtsLayer3UeHandleCallConnect
// LAYER      :: Layer 3
// PURPOSE    :: Handle a CC CALL CONNECT MESSAGE
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + ueL3           : UmtsLayer3Ue *         : Pointer to UMTS Layer3 data
// + msg            : char*    : Pointer to the NAS message contents
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3UeHandleCallConnect(
        Node* node,
        UmtsLayer3Data* umtsL3,
        UmtsLayer3UeAppInfo* ueFlow,
        char* msg,
        int msgSize)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*)(umtsL3->dataPtr);
    if (DEBUG_CC)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_CC);
        printf("\n\t UE: %d rcvd a CONNECT from the MSC\n",
                node->nodeId);
    }
    ueL3->stat.numCcConnectRcvd++;
    if (ueFlow)
    {
        UmtsCcMsData* ueCc = ueFlow->cmData.ccData;
        if (ueCc->state == UMTS_CC_MS_CALL_INITIATED
            ||ueCc->state == UMTS_CC_MS_MOBILE_ORIGINATING_CALL_PROCEEDING
            ||ueCc->state == UMTS_CC_MS_CALL_DELIVERED)
        {
            if (ueCc->T303)
            {
                MESSAGE_CancelSelfMsg(node, ueCc->T303);
                ueCc->T303 = NULL;
            }
            else if (ueCc->T310)
            {
                MESSAGE_CancelSelfMsg(node, ueCc->T310);
                ueCc->T310 = NULL;
            }

            ueFlow->rabId = msg[0];
            UmtsUeRabInfo* rabInfo = ueL3->rrcData->rabInfos[(int)ueFlow->rabId];
            ERROR_Assert(rabInfo != NULL, "Cannnot find RAB info.");
            ERROR_Assert(rabInfo->rbIds[0] != UMTS_INVALID_RAB_ID,
                "RAB does not contain a RB.");
            ueFlow->rbId = rabInfo->rbIds[0];

            ueCc->state = UMTS_CC_MS_ACTIVE;
            UmtsLayer3UeSendCallConnectAck(node, umtsL3, ueFlow->transId);

            // Notify the umtsapp that call is anwsered
            UmtsLayer3UeNotifyAppCallAnswerred(
                node,
                umtsL3,
                ueFlow);
        }
    }
}

// FUNCTION   :: UmtsLayer3UeHandleCallSetup
// LAYER      :: Layer 3
// PURPOSE    :: Handle a SETUP MESSAGE to establish
//               a mobile terminating message
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + ueL3           : UmtsLayer3Ue *         : Pointer to UMTS Layer3 data
// + msg            : char* : Pointer to the NAS message contents
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3UeHandleCallSetup(
        Node* node,
        UmtsLayer3Data* umtsL3,
        char ti,
        char* msg,
        int msgSize)
{
    if (DEBUG_CC)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_CC);
        printf("\n\t UE: %d rcvd a SETUP from the MSC\n",
                node->nodeId);
    }
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*)(umtsL3->dataPtr);
    ueL3->stat.numMtAppRcvdFromNetwork ++;
    ueL3->stat.numMtCsAppRcvdFromNetwork ++;
    ueL3->stat.numCcSetupRcvd++;

    UmtsLayer3UeAppInfo* ueFlow = UmtsLayer3UeAddAppInfoToList(
                                           node,
                                           ueL3,
                                           UMTS_FLOW_NETWORK_ORIGINATED,
                                           UMTS_LAYER3_CN_DOMAIN_CS,
                                           ti);
    UmtsCcMsData* ueCc = ueFlow->cmData.ccData;
    ueCc->state = UMTS_CC_MS_CALL_PRESENT;
    int index = 0;
    memcpy(&(ueCc->peerAddr), msg, sizeof(ueCc->peerAddr));
    index += sizeof(ueCc->peerAddr);
    memcpy(&(ueCc->callAppId), msg + index, sizeof(ueCc->callAppId));
    index += sizeof(ueCc->callAppId);
    memcpy(&(ueCc->endTime), msg + index, sizeof(ueCc->endTime));
    index += sizeof(ueCc->endTime);
    memcpy(&(ueCc->avgTalkTime), msg + index, sizeof(ueCc->avgTalkTime));

    UmtsLayer3UeSendCallConfirm(node, umtsL3, ueFlow);

    // Send Alerting to the UmtsCall application
    UmtsLayer3UeAlertAppCall(node, umtsL3, ueFlow);

    UmtsLayer3UeSendAlertingToMsc(node, umtsL3, ueFlow);

    ueL3->stat.numMtAppInited ++;
    ueL3->stat.numMtCsAppInited ++;
}

// FUNCTION   :: UmtsLayer3UeHandleCallConnAck
// LAYER      :: Layer 3
// PURPOSE    :: Handle a CC  CONNECT ACKNOWLEDGE MESSAGE
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + ueL3           : UmtsLayer3Ue *         : Pointer to UMTS Layer3 data
// + msg            : char*  : Pointer to the NAS message contents
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3UeHandleCallConnAck(
        Node* node,
        UmtsLayer3Data* umtsL3,
        UmtsLayer3UeAppInfo* ueFlow,
        char* msg,
        int msgSize)
{
    if (DEBUG_CC)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_CC);
        printf("\n\t UE: %d rcvd a CONNECT ACK from the MSC\n",
                node->nodeId);
    }
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*)(umtsL3->dataPtr);
    ueL3->stat.numCcConnAckRcvd++;

    if (ueFlow && ueFlow->cmData.ccData->state
                            == UMTS_CC_MS_CONNECT_REQUEST)
    {
        UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*)(umtsL3->dataPtr);
        UmtsCcMsData* ueCc = ueFlow->cmData.ccData;
        if (ueCc->T313)
        {
            MESSAGE_CancelSelfMsg(node, ueCc->T313);
            ueCc->T313 = NULL;
        }
        ueFlow->rabId = msg[0];
        UmtsUeRabInfo* rabInfo = ueL3->rrcData->rabInfos[(int)ueFlow->rabId];
        ERROR_Assert(rabInfo != NULL, "Cannnot find RAB info.");
        ERROR_Assert(rabInfo->rbIds[0] != UMTS_INVALID_RAB_ID,
            "RAB does not contain a RB.");
        ueFlow->rbId = rabInfo->rbIds[0];

        ueCc->state = UMTS_CC_MS_ACTIVE;

       // No need to send a CALL_ACCEPT message to the umtscall app
       // since it's a mobile terminated call?
    }
}

// FUNCTION   :: UmtsLayer3UeHandleCallDisconnect
// LAYER      :: Layer 3
// PURPOSE    :: Handle a CC DISCONNECT MESSAGE
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + ueL3           : UmtsLayer3Ue *         : Pointer to UMTS Layer3 data
// + msg            : char*  : Pointer to the NAS message contents
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3UeHandleCallDisconnect(
        Node* node,
        UmtsLayer3Data* umtsL3,
        UmtsLayer3UeAppInfo* ueFlow,
        char* msg,
        int msgSize)
{
    if (DEBUG_CC)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_CC);
        printf("\n\t UE: %d rcvd a DISCONNECT from the MSC\n",
                node->nodeId);
    }
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*)(umtsL3->dataPtr);
    ueL3->stat.numCcDisconnectRcvd++;

    if (ueFlow && ueFlow->cmData.ccData->state != UMTS_CC_MS_NULL
           && ueFlow->cmData.ccData->state != UMTS_CC_MS_RELEASE_REQUEST)
    {
        UmtsCcMsData* ueCc = ueFlow->cmData.ccData;

        UmtsLayer3UeStopCcTimers(node, ueL3, ueCc);
        if (ueCc->T305)
        {
            MESSAGE_CancelSelfMsg(node, ueCc->T305);
            ueCc->T305 = NULL;
        }
        UmtsLayer3UeSendCallRelease(node, umtsL3, ueFlow);
        //If UE has already sent a DISCONNECT, it doesn't need to report
        //to APP that the call has ended.
        if (ueFlow->cmData.ccData->state != UMTS_CC_MS_DISCONNECT_REQUEST)
        {
            ueCc->T308 = UmtsLayer3SetTimer(
                                node,
                                umtsL3,
                                UMTS_LAYER3_TIMER_CC_T308,
                                UMTS_LAYER3_TIMER_CC_T308_MS_VAL,
                                NULL,
                                (void*)&(ueFlow->transId),
                                sizeof(ueFlow->transId));
            ueCc->state = UMTS_CC_MS_RELEASE_REQUEST;
            ueCc->counterT308 = 0;

            UmtsCallClearingType clearType = (UmtsCallClearingType)(*msg);
            UmtsLayer3UeNotifyAppCallClearing(
                    node,
                    umtsL3,
                    ueFlow,
                    clearType);
        }
    }
}

// FUNCTION   :: UmtsLayer3UeHandleCallRelease
// LAYER      :: Layer 3
// PURPOSE    :: Handle a CC RELEASE MESSAGE
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + ueL3           : UmtsLayer3Ue *         : Pointer to UMTS Layer3 data
// + msg            : char*  : Pointer to the NAS message contents
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3UeHandleCallRelease(
        Node* node,
        UmtsLayer3Data* umtsL3,
        UmtsLayer3UeAppInfo* ueFlow,
        char* msg,
        int msgSize)
{
    if (DEBUG_CC)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_CC);
        printf("\n\t UE: %d rcvd a RELEASE from the MSC\n",
                node->nodeId);
    }
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*)(umtsL3->dataPtr);
    ueL3->stat.numCcReleaseRcvd++;

    if (ueFlow && ueFlow->cmData.ccData->state != UMTS_CC_MS_NULL)
    {
        UmtsCcMsData* ueCc = ueFlow->cmData.ccData;

        UmtsLayer3UeStopCcTimers(node, ueL3, ueCc);
        if (ueCc->T305)
        {
            MESSAGE_CancelSelfMsg(node, ueCc->T305);
            ueCc->T305 = NULL;
        }

        //If the UE isn't expecting the RELEASE, the call clearing is
        //initiated from the network, need to notify the application
        if (ueCc->state != UMTS_CC_MS_DISCONNECT_REQUEST &&
            ueCc->state != UMTS_CC_MS_RELEASE_REQUEST)
        {
            UmtsCallClearingType clearType = (UmtsCallClearingType)(*msg);
            UmtsLayer3UeNotifyAppCallClearing(
                    node,
                    umtsL3,
                    ueFlow,
                    clearType);
        }

        //No need to stop T308, since it won't be started if not in state
        //RELEASE_REQUEST
        UmtsLayer3UeSendCallReleaseComplete(node, umtsL3, ueFlow);
        ueCc->state = UMTS_CC_MS_NULL;
        char ti = ueFlow->transId;
        UmtsLayer3UeRemoveAppInfoFromList(
            node,
            ueL3,
            ueFlow->srcDestType,
            ueFlow->transId);
        UmtsLayer3UeReleaseMmConn(node,
                                  umtsL3,
                                  ueL3,
                                  UMTS_LAYER3_PD_CC,
                                  ti);
    }
}

// FUNCTION   :: UmtsLayer3UeHandleCallReleaseComplete
// LAYER      :: Layer 3
// PURPOSE    :: Handle a CC RELEASE COMPLETE MESSAGE
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + ueL3           : UmtsLayer3Ue *         : Pointer to UMTS Layer3 data
// + msg            : char*   : Pointer to the NAS message contents
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3UeHandleCallReleaseComplete(
        Node* node,
        UmtsLayer3Data* umtsL3,
        UmtsLayer3UeAppInfo* ueFlow,
        char* msg,
        int msgSize)
{
    if (DEBUG_CC)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_CC);
        printf("\n\t UE: %d rcvd a RELEASE COMPLETE from the MSC\n",
                node->nodeId);
    }
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*)(umtsL3->dataPtr);
    ueL3->stat.numCcReleaseCompleteRcvd++;

    if (ueFlow)
    {
        UmtsCcMsData* ueCc = ueFlow->cmData.ccData;

        UmtsLayer3UeStopCcTimers(node, ueL3, ueCc);
        if (ueCc->T305)
        {
            MESSAGE_CancelSelfMsg(node, ueCc->T305);
            ueCc->T305 = NULL;
        }
        if (ueCc->T308)
        {
            MESSAGE_CancelSelfMsg(node, ueCc->T308);
            ueCc->T308 = NULL;
        }
        ueCc->state = UMTS_CC_MS_NULL;
        char ti = ueFlow->transId;
        UmtsLayer3UeRemoveAppInfoFromList(
            node,
            ueL3,
            ueFlow->srcDestType,
            ueFlow->transId);
        UmtsLayer3UeReleaseMmConn(node,
                                  umtsL3,
                                  ueL3,
                                  UMTS_LAYER3_PD_CC,
                                  ti);
    }
}

// FUNCTION   :: UmtsLayer3UeHandleCcMsg
// LAYER      :: Layer 3
// PURPOSE    :: Handle a CC message
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data         : Pointer to UMTS Layer3 data
// + msg            : char*  : Pointer to the NAS message contents
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3UeHandleCcMsg(
        Node* node,
        UmtsLayer3Data* umtsL3,
        UmtsCcMessageType msgType,
        char ti,
        char* msg,
        int msgSize)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*) umtsL3->dataPtr;
    UmtsLayer3UeAppInfo* ueFlow = UmtsLayer3UeFindAppInfoFromList(
                                    node,
                                    ueL3,
                                    ti);
    switch (msgType)
    {
        //Mobile originating call messages
        case UMTS_CC_MSG_TYPE_CALL_PROCEEDING:
        {
            UmtsLayer3UeHandleCallProc(
                node, umtsL3, ueFlow, msg, msgSize);
            break;
        }
        case UMTS_CC_MSG_TYPE_ALERTING:
        {
            UmtsLayer3UeHandleCallAlerting(
                node, umtsL3, ueFlow, msg, msgSize);
            break;
        }
        case UMTS_CC_MSG_TYPE_CONNECT:
        {
            UmtsLayer3UeHandleCallConnect(
                node, umtsL3, ueFlow, msg, msgSize);
            break;
        }
        //Mobile terminating call messages
        case UMTS_CC_MSG_TYPE_SETUP:
        {
            if (ueFlow == NULL)
            {
                UmtsLayer3UeHandleCallSetup(
                    node, umtsL3, ti,  msg, msgSize);
            }
            break;
        }
        case UMTS_CC_MSG_TYPE_CONNECT_ACKNOWLEDGE:
        {
            UmtsLayer3UeHandleCallConnAck(
                    node, umtsL3, ueFlow,  msg, msgSize);
            break;
        }
        //Call clearing messages
        case UMTS_CC_MSG_TYPE_DISCONNECT:
        {
            UmtsLayer3UeHandleCallDisconnect(
                    node, umtsL3, ueFlow,  msg, msgSize);
            break;
        }
        case UMTS_CC_MSG_TYPE_RELEASE:
        {
            UmtsLayer3UeHandleCallRelease(
                    node, umtsL3, ueFlow,  msg, msgSize);
            break;
        }
        case UMTS_CC_MSG_TYPE_RELEASE_COMPLETE:
        {
            UmtsLayer3UeHandleCallReleaseComplete(
                    node, umtsL3, ueFlow,  msg, msgSize);
            break;
        }
        default:
            ERROR_ReportError("Received an unidentified CC message.");
    }
}

//
// handle SM Msg
//
// /**
// FUNCTION   :: UmtsLayer3UeSmHandleDeactivatePDPContextAcpt
// LAYER      :: Layer 3
// PURPOSE    :: Handle a deactivate PDP context accept message .
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + transactId     : char                   : Transaction ID
// + msg            : char* : Pointer to the NAS message contents
// + msgSize        : int                    : Message size
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3UeSmHandleDeactivatePDPContextAcpt(
            Node*               node,
            UmtsLayer3Data*     umtsL3,
            unsigned char       transactId,
            char*               msg,
            int                 msgSize)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*) umtsL3->dataPtr;
    if (DEBUG_SM)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_SM);
        printf("\n\tRcvd a deactivate PDP Accept message for tranId %d\n",
               transactId);
    }

    // update stat
    ueL3->stat.numDeactivatePDPContextAcptRcvd ++;

    UmtsLayer3UeAppInfo* appInfo;
    UmtsFlowSrcDestType srcDestType;

    if (transactId >= UMTS_LAYER3_NETWORK_MOBILE_START)
    {
        srcDestType = UMTS_FLOW_NETWORK_ORIGINATED;
    }
    else
    {
        srcDestType = UMTS_FLOW_MOBILE_ORIGINATED;
    }

    // update SM state
    appInfo = UmtsLayer3UeFindAppInfoFromList(
        node, ueL3, srcDestType, transactId);
    ERROR_Assert(appInfo != NULL,
                 "no appInfo link with this transId and srcDestType");

    appInfo->cmData.smData->smState = UMTS_SM_MS_PDP_INACTIVE;


    // cancel T3390
    if (appInfo->cmData.smData->T3390 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, appInfo->cmData.smData->T3390);
        appInfo->cmData.smData->T3390 = NULL;
    }

    UmtsLayer3UeRemoveAppInfoFromList(node,
                                      ueL3,
                                      appInfo->srcDestType,
                                      appInfo->transId);

    if (0 == std::count_if(ueL3->appInfoList->begin(),
                           ueL3->appInfoList->end(),
                           UmtsCheckFlowDomain(UMTS_LAYER3_CN_DOMAIN_PS)))
    {
        ueL3->gmmData->pmmState = UMTS_PMM_IDLE;
        UmtsLayer3UeSendSignalConnRelIndi(
            node,
            umtsL3,
            ueL3,
            UMTS_LAYER3_CN_DOMAIN_PS);

        // update stat
        ueL3->stat.numSigConnRelIndSent ++;
    }
}

// /**
// FUNCTION   :: UmtsLayer3UeSmHandleDeactivatePDPContextRequest
// LAYER      :: Layer 3
// PURPOSE    :: Handle a deactivate PDP context request message
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + transactId     : char                   : Transaction ID
// + msg            : char* : Pointer to the NAS message contents
// + msgSize        : int                    : Message size
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3UeSmHandleDeactivatePDPContextRequest(
            Node*               node,
            UmtsLayer3Data*     umtsL3,
            unsigned char        transactId,
            char*               msg,
            int                 msgSize)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*) umtsL3->dataPtr;
    UmtsLayer3UeAppInfo* appInfo;
    UmtsFlowSrcDestType srcDestType;
    UmtsSmCauseType deactCause;
    UmtsLayer3DeactivatePDPContextRequest pdpReqMsg;

    if (DEBUG_SM)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_SM);
        printf("\n\tRcvd a deactivate PDP Request message for tranId %d\n",
               transactId);
    }

    // update stat
    ueL3->stat.numDeactivatePDPContextRequestRcvd ++;

    if (transactId >= UMTS_LAYER3_NETWORK_MOBILE_START)
    {
        srcDestType = UMTS_FLOW_NETWORK_ORIGINATED;
    }
    else
    {
        srcDestType = UMTS_FLOW_MOBILE_ORIGINATED;
    }

    // update SM state
    appInfo = UmtsLayer3UeFindAppInfoFromList(
                node,
                ueL3,
                srcDestType,
                transactId);

    // Send Deactivate PDP Context Response to network
    UmtsLayer3UeSmSendDeactivatePDPContextAccept(
        node, umtsL3, ueL3, transactId);

    if (!appInfo)
    {
        return;
    }

    memcpy(&pdpReqMsg, msg, sizeof(pdpReqMsg));
    deactCause = (UmtsSmCauseType) pdpReqMsg.smCause;
    // cancel T3390
    if (appInfo->cmData.smData->T3390 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, appInfo->cmData.smData->T3390);
        appInfo->cmData.smData->T3390 = NULL;
    }

    // update stat
    ueL3->stat.numDeactivatePDPContextAcptSent ++;

    // update the SM state
    appInfo->cmData.smData->smState = UMTS_SM_MS_PDP_INACTIVE;

    // update stats
    // update app leve stat
    if (deactCause == UMTS_SM_CAUSE_REGUALR_DEACTIVATION)
    {
        if (appInfo->srcDestType == UMTS_FLOW_NETWORK_ORIGINATED)
        {
            ueL3->stat.numMtAppSucEnded ++;

            if (appInfo->qosInfo->trafficClass ==
                    UMTS_QOS_CLASS_CONVERSATIONAL)
            {
                ueL3->stat.numMtConversationalAppSucEnded ++;
            }
            else if (appInfo->qosInfo->trafficClass ==
                UMTS_QOS_CLASS_STREAMING)
            {
                ueL3->stat.numMtStreamingAppSucEnded ++;
            }
            else if (appInfo->qosInfo->trafficClass ==
                UMTS_QOS_CLASS_BACKGROUND)
            {
                ueL3->stat.numMtBackgroundAppSucEnded ++;
            }
            else if (appInfo->qosInfo->trafficClass >
                UMTS_QOS_CLASS_STREAMING &&
                appInfo->qosInfo->trafficClass <
                UMTS_QOS_CLASS_BACKGROUND)
            {
                ueL3->stat.numMtInteractiveAppSucEnded ++;
            }
        }
        else if (appInfo->srcDestType == UMTS_FLOW_MOBILE_ORIGINATED)
        {
            ueL3->stat.numMoAppSucEnded ++;
            if (appInfo->qosInfo->trafficClass ==
                    UMTS_QOS_CLASS_CONVERSATIONAL)
            {
                ueL3->stat.numMoConversationalAppSucEnded ++;
            }
            else if (appInfo->qosInfo->trafficClass ==
                UMTS_QOS_CLASS_STREAMING)
            {
                ueL3->stat.numMoStreamingAppSucEnded ++;
            }
            else if (appInfo->qosInfo->trafficClass ==
                UMTS_QOS_CLASS_BACKGROUND)
            {
                ueL3->stat.numMoBackgroundAppSucEnded ++;
            }
            else if (appInfo->qosInfo->trafficClass >
                UMTS_QOS_CLASS_STREAMING &&
                appInfo->qosInfo->trafficClass <
                UMTS_QOS_CLASS_BACKGROUND)
            {
                ueL3->stat.numMoInteractiveAppSucEnded ++;
            }
        }
    }
    else
    {
        if (appInfo->srcDestType == UMTS_FLOW_NETWORK_ORIGINATED)
        {
            ueL3->stat.numMtAppDropped ++;
            if (appInfo->qosInfo->trafficClass ==
                    UMTS_QOS_CLASS_CONVERSATIONAL)
            {
                ueL3->stat.numMtConversationalAppDropped ++;
            }
            else if (appInfo->qosInfo->trafficClass ==
                UMTS_QOS_CLASS_STREAMING)
            {
                ueL3->stat.numMtStreamingAppDropped ++;
            }
            else if (appInfo->qosInfo->trafficClass ==
                UMTS_QOS_CLASS_BACKGROUND)
            {
                ueL3->stat.numMtBackgroundAppDropped ++;
            }
            else if (appInfo->qosInfo->trafficClass >
                UMTS_QOS_CLASS_STREAMING &&
                appInfo->qosInfo->trafficClass <
                UMTS_QOS_CLASS_BACKGROUND)
            {
                ueL3->stat.numMtInteractiveAppDropped ++;
            }
        }
        else if (appInfo->srcDestType == UMTS_FLOW_MOBILE_ORIGINATED)
        {
            ueL3->stat.numMoAppDropped ++;
            if (appInfo->qosInfo->trafficClass ==
                    UMTS_QOS_CLASS_CONVERSATIONAL)
            {
                ueL3->stat.numMoConversationalAppDropped ++;
            }
            else if (appInfo->qosInfo->trafficClass ==
                UMTS_QOS_CLASS_STREAMING)
            {
                ueL3->stat.numMoStreamingAppDropped ++;
            }
            else if (appInfo->qosInfo->trafficClass ==
                UMTS_QOS_CLASS_BACKGROUND)
            {
                ueL3->stat.numMoBackgroundAppDropped ++;
            }
            else if (appInfo->qosInfo->trafficClass >
                UMTS_QOS_CLASS_STREAMING &&
                appInfo->qosInfo->trafficClass <
                UMTS_QOS_CLASS_BACKGROUND)
            {
                ueL3->stat.numMoInteractiveAppDropped ++;
            }
        }
    }

    // remove the flow from list
    UmtsLayer3UeRemoveAppInfoFromList(node,
                                      ueL3,
                                      appInfo->srcDestType,
                                      appInfo->transId);
}

// /**
// FUNCTION   :: UmtsLayer3UeSmHandleActivatePDPContextAcpt
// LAYER      :: Layer 3
// PURPOSE    :: Handle a activate PDP context accept message .
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + transactId     : char                   : Transaction ID
// + msg            : char* : Pointer to the NAS message contents
// + msgSize        : int                    : Message size
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3UeSmHandleActivatePDPContextAcpt(
            Node*               node,
            UmtsLayer3Data*     umtsL3,
            unsigned char       transactId,
            char*               msg,
            int                 msgSize)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*) umtsL3->dataPtr;
    if (DEBUG_SM)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_SM);
        printf("\n\t Receives a activate PDP Accept message\n");
    }

    // update stat
    ueL3->stat.numActivatePDPContextAcptRcvd ++;

    UmtsLayer3UeAppInfo* appInfo;
    UmtsFlowSrcDestType srcDestType;

    if (transactId >= UMTS_LAYER3_NETWORK_MOBILE_START)
    {
        srcDestType = UMTS_FLOW_NETWORK_ORIGINATED;
    }
    else
    {
        srcDestType = UMTS_FLOW_MOBILE_ORIGINATED;
    }

    // update SM state
    appInfo = UmtsLayer3UeFindAppInfoFromList(
        node, ueL3, srcDestType, transactId);

    if (DEBUG_ASSERT)
    {
        ERROR_Assert(appInfo != NULL,
            "no appInfo link with this transId and srcDestType");
    }
    if (!appInfo)
    {
        return;
    }
    appInfo->cmData.smData->smState = UMTS_SM_MS_PDP_ACTIVE;

    // cancel T3380
    if (appInfo->cmData.smData->T3380 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, appInfo->cmData.smData->T3380);
        appInfo->cmData.smData->T3380 = NULL;
    }

    // configurate the rb as well
    UmtsLayer3ActivatePDPContextAccept* pdpAcptMsg =
        (UmtsLayer3ActivatePDPContextAccept*) msg;

    appInfo->rabId = (char)pdpAcptMsg->sapi;
    UmtsUeRabInfo* rabInfo = ueL3->rrcData->rabInfos[(int)appInfo->rabId];
    ERROR_Assert(rabInfo != NULL, "Cannnot find RAB info.");
    ERROR_Assert(rabInfo->rbIds[0] != UMTS_INVALID_RAB_ID,
        "RAB does not contain a RB.");
    appInfo->rbId = rabInfo->rbIds[0];

    // Send the packet in waiting buffer to rlc
    if (srcDestType == UMTS_FLOW_MOBILE_ORIGINATED)
    {
        Message* msgInBuffer = NULL;

        while (!appInfo->inPacketBuf->empty())
        {
            msgInBuffer = appInfo->inPacketBuf->front();
            appInfo->inPacketBuf->pop();
            ueL3->stat.numPsDataPacketFromUpperDeQueue ++;
            UmtsLayer3UeSendPacketToLayer2(node,
                                       umtsL3->interfaceIndex,
                                       appInfo->rbId,
                                       msgInBuffer);

            // update stat
            ueL3->stat.numPsDataPacketSentToL2 ++;
        }
        appInfo->pktBufSize = 0;
    }

    // update stats
    if (srcDestType == UMTS_FLOW_MOBILE_ORIGINATED)
    {
        // total apps( PS & CS)
        ueL3->stat.numMoAppInited ++;

        // PS

        if (appInfo->qosInfo->trafficClass ==
                UMTS_QOS_CLASS_CONVERSATIONAL)
        {
            ueL3->stat.numMoConversationalAppInited ++;
        }
        else if (appInfo->qosInfo->trafficClass ==
            UMTS_QOS_CLASS_STREAMING)
        {
            ueL3->stat.numMoStreamingAppInited ++;
        }
        else if (appInfo->qosInfo->trafficClass ==
            UMTS_QOS_CLASS_BACKGROUND)
        {
            ueL3->stat.numMoBackgroundAppInited ++;
        }
        else if (appInfo->qosInfo->trafficClass >
            UMTS_QOS_CLASS_STREAMING &&
            appInfo->qosInfo->trafficClass <
            UMTS_QOS_CLASS_BACKGROUND)
        {
            ueL3->stat.numMoInteractiveAppInited ++;
        }
    }
    else if (srcDestType == UMTS_FLOW_NETWORK_ORIGINATED)
    {
        // total apps( PS & CS)
        ueL3->stat.numMtAppInited ++;

        // PS

        if (appInfo->qosInfo->trafficClass ==
                UMTS_QOS_CLASS_CONVERSATIONAL)
        {
            ueL3->stat.numMtConversationalAppInited ++;
        }
        else if (appInfo->qosInfo->trafficClass ==
            UMTS_QOS_CLASS_STREAMING)
        {
            ueL3->stat.numMtStreamingAppInited ++;
        }
        else if (appInfo->qosInfo->trafficClass ==
            UMTS_QOS_CLASS_BACKGROUND)
        {
            ueL3->stat.numMtBackgroundAppInited ++;
        }
        else if (appInfo->qosInfo->trafficClass >
            UMTS_QOS_CLASS_STREAMING &&
            appInfo->qosInfo->trafficClass <
            UMTS_QOS_CLASS_BACKGROUND)
        {
            ueL3->stat.numMtInteractiveAppInited ++;
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3UeSmHandleActivatePDPContextReject
// LAYER      :: Layer 3
// PURPOSE    :: Handle a activate PDP context reject message .
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + transactId     : char                   : Transaction ID
// + msg            : char* : Pointer to the NAS message contents
// + msgSize        : int                    : Message size
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3UeSmHandleActivatePDPContextReject(
         Node*               node,
         UmtsLayer3Data*     umtsL3,
         unsigned char       transactId,
         char*               msg,
         int                 msgSize)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*) umtsL3->dataPtr;
    UmtsLayer3UeAppInfo* appInfo;
    UmtsFlowSrcDestType srcDestType;

    if (DEBUG_SM)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_SM);
        printf("\n\tReceives a activate PDP Rej msg with TrId%d\n",
            transactId);
    }

    // update stat
    ueL3->stat.numActivatePDPContextRejRcvd ++;

    if (transactId >= UMTS_LAYER3_NETWORK_MOBILE_START)
    {
        srcDestType = UMTS_FLOW_NETWORK_ORIGINATED;
    }
    else
    {
        srcDestType = UMTS_FLOW_MOBILE_ORIGINATED;
    }

    // update SM state
    appInfo = UmtsLayer3UeFindAppInfoFromList(
        node, ueL3, srcDestType, transactId);
    ERROR_Assert(appInfo != NULL,
                 "no appInfo link with this transId and srcDestType");

    appInfo->cmData.smData->smState = UMTS_SM_MS_PDP_REJECTED;

    // cancel T3380
    if (appInfo->cmData.smData->T3380 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, appInfo->cmData.smData->T3380);
        appInfo->cmData.smData->T3380 = NULL;
    }


    // Free packets in waiting buffer
    if (srcDestType == UMTS_FLOW_MOBILE_ORIGINATED)
    {
        Message* msgInBuffer = NULL;

        while (!appInfo->inPacketBuf->empty())
        {
            msgInBuffer = appInfo->inPacketBuf->front();
            appInfo->inPacketBuf->pop();
            ueL3->stat.numPsDataPacketFromUpperDeQueue ++;
            MESSAGE_Free(node, msgInBuffer);
            ueL3->stat.numPsPktDroppedNoResource ++;
        }
        appInfo->pktBufSize = 0;
    }

    // update stats
    if (srcDestType == UMTS_FLOW_MOBILE_ORIGINATED)
    {
        // total apps( PS & CS)
        ueL3->stat.numMoAppRejected ++;

        // PS

        if (appInfo->qosInfo->trafficClass ==
                UMTS_QOS_CLASS_CONVERSATIONAL)
        {
            ueL3->stat.numMoConversationalAppRejected ++;
        }
        else if (appInfo->qosInfo->trafficClass ==
            UMTS_QOS_CLASS_STREAMING)
        {
            ueL3->stat.numMoStreamingAppRejected ++;
        }
        else if (appInfo->qosInfo->trafficClass ==
            UMTS_QOS_CLASS_BACKGROUND)
        {
            ueL3->stat.numMoBackgroundAppRejected ++;
        }
        else if (appInfo->qosInfo->trafficClass >
            UMTS_QOS_CLASS_STREAMING &&
            appInfo->qosInfo->trafficClass <
            UMTS_QOS_CLASS_BACKGROUND)
        {
            ueL3->stat.numMoInteractiveAppRejected ++;
        }
    }
    else if (srcDestType == UMTS_FLOW_NETWORK_ORIGINATED)
    {
        // total apps( PS & CS)
        ueL3->stat.numMtAppRejected ++;

        // PS

        if (appInfo->qosInfo->trafficClass ==
                UMTS_QOS_CLASS_CONVERSATIONAL)
        {
            ueL3->stat.numMtConversationalAppRejected ++;
        }
        else if (appInfo->qosInfo->trafficClass ==
            UMTS_QOS_CLASS_STREAMING)
        {
            ueL3->stat.numMtStreamingAppRejected ++;
        }
        else if (appInfo->qosInfo->trafficClass ==
            UMTS_QOS_CLASS_BACKGROUND)
        {
            ueL3->stat.numMtBackgroundAppRejected ++;
        }
        else if (appInfo->qosInfo->trafficClass >
            UMTS_QOS_CLASS_STREAMING &&
            appInfo->qosInfo->trafficClass <
            UMTS_QOS_CLASS_BACKGROUND)
        {
            ueL3->stat.numMtInteractiveAppRejected ++;
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3UeSmHandleRequestPDPContextActivation
// LAYER      :: Layer 3
// PURPOSE    :: Handle a Request PDP Context Activation message.
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + transactId     : char                   : Transaction ID
// + msg            : char* : Pointer to the NAS message contents
// + msgSize        : int                    : Message size
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3UeSmHandleRequestPDPContextActivation(
         Node*               node,
         UmtsLayer3Data*     umtsL3,
         unsigned char       transactId,
         char*               msg,
         int                 msgSize)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*) umtsL3->dataPtr;
    UmtsLayer3UeAppInfo* flowInfo;
    UmtsFlowSrcDestType srcDestType;
    UmtsLayer3RequestPDPContextActivation pdpReqMsg;

    if (DEBUG_SM)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_SM);
        printf("\n\tReceives a request PDP activation message\n");
    }

    // update stat
    ueL3->stat.numRequestPDPConextActivationRcvd ++;
    memcpy(&pdpReqMsg, msg, sizeof(pdpReqMsg));

    if (ueL3->gmmData->mainState == UMTS_GMM_MS_SERVICE_REQUEST_INITIATED)
    {
        if (ueL3->gmmData->T3317)
        {
            MESSAGE_CancelSelfMsg(node, ueL3->gmmData->T3317);
            ueL3->gmmData->T3317 = NULL;
        }

        // update GMM state
        ueL3->gmmData->mainState = UMTS_GMM_MS_REGISTERED;
        ueL3->gmmData->regSubState = UMTS_GMM_MS_REGISTERED_NORMAL_SERVICE;
        ueL3->gmmData->servReqWaitForRrc = FALSE;

        // move state to PMM-CONNECTED if now it is IDLE
        if (ueL3->gmmData->pmmState != UMTS_PMM_CONNECTED)
        {
            ueL3->gmmData->pmmState = UMTS_PMM_CONNECTED;
        }
    }

    ERROR_Assert(transactId >= UMTS_LAYER3_NETWORK_MOBILE_START,
                 "UMTS: Wrong tranction ID!");
    srcDestType = UMTS_FLOW_NETWORK_ORIGINATED;

    // check if already requested before
    flowInfo = UmtsLayer3UeFindAppInfoFromList(node,
                                               ueL3,
                                               srcDestType,
                                               transactId);

    if (flowInfo == NULL)
    {
        // new one, add a record
        UmtsLayer3FlowClassifier flowClsInfo;
        memset(&flowClsInfo, 0, sizeof(flowClsInfo));
        flowClsInfo.domainInfo = UMTS_LAYER3_CN_DOMAIN_PS;
        flowInfo = UmtsLayer3UeAddAppInfoToList(
                       node,
                       ueL3,
                       UMTS_FLOW_NETWORK_ORIGINATED,
                       UMTS_LAYER3_CN_DOMAIN_PS,
                       transactId,
                       &flowClsInfo);

        // update stat
        ueL3->stat.numMtAppRcvdFromNetwork ++;

        flowInfo->qosInfo->trafficClass =
            (UmtsQoSTrafficClass)pdpReqMsg.trafficClass;

        // PS domain
        if (flowInfo->qosInfo->trafficClass ==
            UMTS_QOS_CLASS_CONVERSATIONAL)
        {
            ueL3->stat.numMtConversationalAppRcvdFromNetwork ++;
        }
        else if (flowInfo->qosInfo->trafficClass ==
            UMTS_QOS_CLASS_STREAMING)
        {
            ueL3->stat.numMtStreamingAppRcvdFromNetwork ++;
        }
        else if (flowInfo->qosInfo->trafficClass ==
            UMTS_QOS_CLASS_BACKGROUND)
        {
            ueL3->stat.numMtBackgroundAppRcvdFromNetwork ++;
        }
        else if (flowInfo->qosInfo->trafficClass >
            UMTS_QOS_CLASS_STREAMING &&
            flowInfo->qosInfo->trafficClass <
            UMTS_QOS_CLASS_BACKGROUND)
        {
            ueL3->stat.numMtInteractiveAppRcvdFromNetwork ++;
        }
    }

    // Now, send Activate PDP Context Request to establish PDP context
    UmtsLayer3UeSmInitActivatePDPContextRequest(
        node,
        umtsL3,
        ueL3,
        flowInfo);
}

// /**
// FUNCTION   :: UmtsGMmHandleTimerT3380
// LAYER      :: Layer 3
// PURPOSE    :: Handle a T3380 timer expired msg
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + ueL3      : UmtsLayer3Ue *   : UE data
// + msg       : Message*         : Message containing the packet
// RETURN     :: BOOL : msg need to be reused or not
// **/
static
BOOL UmtsLayer3UeSmHandleTimerT3380(Node *node,
                                    UmtsLayer3Data *umtsL3,
                                    UmtsLayer3Ue *ueL3,
                                    Message *msg)
{
    BOOL msgReuse = FALSE;
    unsigned char transId;
    UmtsFlowSrcDestType srcDestType;
    UmtsLayer3UeAppInfo* appInfo;
    unsigned char* timerInfo;

    timerInfo = (unsigned char*) MESSAGE_ReturnInfo(msg);

    transId = *(timerInfo + sizeof(UmtsLayer3Timer));
    srcDestType = (UmtsFlowSrcDestType)
                  *(timerInfo + sizeof(UmtsLayer3Timer) + 1);

    if (ueL3->gmmData->pmmState == UMTS_PMM_CONNECTED)
    {
        appInfo = UmtsLayer3UeFindAppInfoFromList(
            node, ueL3, (UmtsFlowSrcDestType)srcDestType, transId);

        if (appInfo != NULL)
        {
            appInfo->cmData.smData->counterT3380++;
            if (appInfo->cmData.smData->counterT3380 >
                          UMTS_LAYER3_TIMER_SM_MAXCOUNTER)
            {
                //Abort the procedure
                appInfo->cmData.smData->smState = UMTS_SM_MS_PDP_REJECTED;

                while (!appInfo->inPacketBuf->empty())
                {
                    MESSAGE_Free(node, appInfo->inPacketBuf->front());
                    appInfo->inPacketBuf->pop();
                    ueL3->stat.numPsPktDroppedNoResource ++;
                    ueL3->stat.numPsDataPacketFromUpperDeQueue ++;
                }
                appInfo->pktBufSize = 0;
                appInfo->cmData.smData->T3380 = NULL;
            }
            else
            {
                UmtsLayer3UeSmSendActivatePDPContextRequest(
                    node,
                    umtsL3,
                    ueL3,
                    srcDestType,
                    transId);
                ueL3->stat.numActivatePDPContextReqSent ++;

                appInfo->cmData.smData->T3380 =
                        UmtsLayer3SetTimer(
                                 node,
                                 umtsL3,
                                 UMTS_LAYER3_TIMER_T3380,
                                 UMTS_LAYER3_TIMER_T3380_VAL,
                                 msg,
                                 NULL,
                                 0);
                msgReuse = TRUE;
            }
        }
    }
    return msgReuse;
}

// /**
// FUNCTION   :: UmtsGMmHandleTimerT3390
// LAYER      :: Layer 3
// PURPOSE    :: Handle a T3390 timer expired msg
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + ueL3      : UmtsLayer3Ue *   : UE data
// + msg       : Message*         : Message containing the packet
// RETURN     :: BOOL : msg need to be reused or not
// **/
static
BOOL UmtsLayer3UeSmHandleTimerT3390(Node *node,
                                    UmtsLayer3Data *umtsL3,
                                    UmtsLayer3Ue *ueL3,
                                    Message *msg)
{
    BOOL msgReuse = FALSE;
    unsigned char transId;
    UmtsFlowSrcDestType srcDestType;
    UmtsLayer3UeAppInfo* appInfo;
    unsigned char* timerInfo;
    unsigned char smCause;

    timerInfo = (unsigned char*) MESSAGE_ReturnInfo(msg);

    transId = *(timerInfo + sizeof(UmtsLayer3Timer));
    srcDestType = (UmtsFlowSrcDestType)
                  *(timerInfo + sizeof(UmtsLayer3Timer) + 1);
    smCause = *(timerInfo+ sizeof(UmtsLayer3Timer) + 2);

    if (ueL3->gmmData->pmmState == UMTS_PMM_CONNECTED)
    {
        appInfo = UmtsLayer3UeFindAppInfoFromList(
                node, ueL3, (UmtsFlowSrcDestType)srcDestType, transId);
        ERROR_Assert(appInfo != NULL,
                     "no appInfo link with this transId and srcDestType");

        appInfo->cmData.smData->counterT3390 ++;
        if (appInfo->cmData.smData->counterT3390 >
                    UMTS_LAYER3_TIMER_SM_MAXCOUNTER)
        {
            appInfo->cmData.smData->T3390 = NULL;
            UmtsLayer3UeRemoveAppInfoFromList(node,
                                              ueL3,
                                              srcDestType,
                                              transId);

            if (0 == std::count_if(
                        ueL3->appInfoList->begin(),
                        ueL3->appInfoList->end(),
                        UmtsCheckFlowDomain(UMTS_LAYER3_CN_DOMAIN_PS)))
            {
                ueL3->gmmData->pmmState = UMTS_PMM_IDLE;
                UmtsLayer3UeSendSignalConnRelIndi(
                    node,
                    umtsL3,
                    ueL3,
                    UMTS_LAYER3_CN_DOMAIN_PS);

                // update stat
                ueL3->stat.numSigConnRelIndSent ++;
            }
        }
        else
        {
            UmtsLayer3UeSmSendDeactivatePDPContextRequest(
                node, umtsL3, ueL3, (UmtsFlowSrcDestType)srcDestType,
                transId, (UmtsSmCauseType)smCause);
            ueL3->stat.numDeactivatePDPContextReqSent ++;

            appInfo->cmData.smData->T3390 =
                UmtsLayer3SetTimer(
                    node, umtsL3, UMTS_LAYER3_TIMER_T3390,
                    UMTS_LAYER3_TIMER_T3390_VAL, msg,
                    NULL, 0);

            msgReuse = TRUE;
        }
    }
    return msgReuse;
}

// /**
// FUNCTION   :: UmtsLayer3UeSmInitDeactivatePDPContext
// LAYER      :: Layer 3
// PURPOSE    :: init deactivate PDP context .
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + transactId     : char                   : Transaction ID
// + srcDestType    : UmtsFlowSrcDestType     : src dest type
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3UeSmInitDeactivatePDPContext(
            Node*               node,
            UmtsLayer3Data*     umtsL3,
            UmtsLayer3Ue*       ueL3,
            unsigned char       transactId,
            UmtsFlowSrcDestType srcDestType,
            UmtsSmCauseType smCasue)
{
    UmtsLayer3UeAppInfo* appInfo;

    // TODO
    appInfo = UmtsLayer3UeFindAppInfoFromList(
            node, ueL3, srcDestType, transactId);
    ERROR_Assert(appInfo != NULL,
                 "no appInfo link with this transId and srcDestType");

    appInfo->cmData.smData->counterT3390 = 0;
    UmtsLayer3UeSmSendDeactivatePDPContextRequest(
        node, umtsL3, ueL3, srcDestType, transactId, smCasue);
    ueL3->stat.numDeactivatePDPContextReqSent ++;


    appInfo->cmData.smData->smState = UMTS_SM_MS_PDP_INACTIVE_PENDING;

    // start T3390
    if (appInfo->cmData.smData->T3390 != NULL)
    {
        MESSAGE_CancelSelfMsg(node, appInfo->cmData.smData->T3390);
        appInfo->cmData.smData->T3390 = NULL;
    }
    unsigned char info[3];
    info[0] = appInfo->transId;
    info[1] = (unsigned char)appInfo->srcDestType;
    info[2] = (unsigned char)smCasue;

    appInfo->cmData.smData->T3390 =
            UmtsLayer3SetTimer(
                node, umtsL3, UMTS_LAYER3_TIMER_T3390,
                UMTS_LAYER3_TIMER_T3390_VAL, NULL,
                (void*)&info[0], 3);
}

// /**
// FUNCTION   :: UmtsLayer3UeHandleSmMsg
// LAYER      :: Layer 3
// PURPOSE    :: Handle a GPRS Mobility Management message from the network .
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msgType        : UmtsGMmMessageType      : Message type
// + transactId     : char                   : Transaction ID
// + msg            : char*  : Pointer to the NAS message contents
// + msgSize        : int                    : Message size
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3UeHandleSmMsg(
    Node*               node,
    UmtsLayer3Data*     umtsL3,
    UmtsSmMessageType   msgType,
    unsigned char       transactId,
    char*               msg,
    int                 msgSize)
{
    switch (msgType)
    {
        case UMTS_SM_MSG_TYPE_ACTIVATE_PDP_ACCEPT:
        {
            UmtsLayer3UeSmHandleActivatePDPContextAcpt(
                node,
                umtsL3,
                transactId,
                msg,
                msgSize);
            break;
        }
        case UMTS_SM_MSG_TYPE_DEACTIVATE_PDP_ACCEPT:
        {
            UmtsLayer3UeSmHandleDeactivatePDPContextAcpt(
                node,
                umtsL3,
                transactId,
                msg,
                msgSize);
            break;
        }
        case UMTS_SM_MSG_TYPE_ACTIVATE_PDP_REJECT:
        {
            UmtsLayer3UeSmHandleActivatePDPContextReject(
                node,
                umtsL3,
                transactId,
                msg,
                msgSize);
            break;
        }
        case UMTS_SM_MSG_TYPE_REQUEST_PDP_ACTIVATION:
        {
            UmtsLayer3UeSmHandleRequestPDPContextActivation(
                node,
                umtsL3,
                transactId,
                msg,
                msgSize);
            break;
        }
        case UMTS_SM_MSG_TYPE_DEACTIVATE_PDP_REQUEST:
        {
            UmtsLayer3UeSmHandleDeactivatePDPContextRequest(
                node,
                umtsL3,
                transactId,
                msg,
                msgSize);
            break;
        }
        default:
        {
            char errStr[MAX_STRING_LENGTH];
            sprintf(errStr,
                    "UMTS Layer3: UE: %d receivers a SM "
                    "signalling msg with unknown type: %d",
                    node->nodeId,
                    msgType);
            ERROR_ReportWarning(errStr);
        }
    }
}

//
// handle CC Msg
//
//
// Power On /Off
//
// /**
// FUNCTION   :: UmtsLayer3UePowerOn
// LAYER      :: Layer 3
// PURPOSE    :: UE is powered on. Do all required actions
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UePowerOn(Node *node, UmtsLayer3Data *umtsL3)
{
    UmtsMmUeUpdateMmState(node, umtsL3,
        (UmtsLayer3Ue*)umtsL3->dataPtr, UMTS_MM_UPDATE_PowerOn);

    // currently poweron is not supported yet
    if (0)
    {
        // if GPPRS is supported
        UmtsGMmUeUpdateGMmState(node, umtsL3,
            (UmtsLayer3Ue*)umtsL3->dataPtr, UMTS_GMM_UPDATE_EnableGprsMode);
    }
    // TODO:  more......
}

// /**
// FUNCTION   :: UmtsLayer3UeHandleAppTimeout
// LAYER      :: Layer3 RRC
// PURPOSE    :: Notify MM/GMM whether the RRC connection has been established
// PARAMETERS ::
// + node      : Node*               : Pointer to node.
// + umtsL3    : UmtsLayer3Data *    : Pointer to UMTS Layer3 data
// + ueL3      : UmtsLayer3Ue*       : Pointer to UE layer3
// + msg       : Message*            : Mesaget to be handled
// RETURN      : BOOL                : reuse the msg or not
// **/
static
BOOL UmtsLayer3UeHandleAppTimeout(Node* node,
                                  UmtsLayer3Data* umtsL3,
                                  UmtsLayer3Ue* ueL3,
                                  Message* msg)
{
    std::list<UmtsLayer3UeAppInfo*>::iterator it;
    UmtsLayer3UeAppInfo* appInfo;
    clocktype curTime = node->getNodeTime();

    for (it = ueL3->appInfoList->begin();
        it != ueL3->appInfoList->end();
        it ++)
    {
        appInfo = (*it);
        if (appInfo->srcDestType == UMTS_FLOW_NETWORK_ORIGINATED)
        {
            continue;
        }

        if (appInfo->classifierInfo->domainInfo ==
              UMTS_LAYER3_CN_DOMAIN_PS)
        {
            // check if sm state is idle?
            if (appInfo->cmData.smData->smState == UMTS_SM_MS_PDP_ACTIVE
                && (appInfo->lastActiveTime
                + UMTS_DEFAULT_PS_FLOW_IDLE_TIME <
                curTime))
            {
                UmtsLayer3UeSmInitDeactivatePDPContext(
                    node, umtsL3, ueL3,
                    appInfo->transId, appInfo->srcDestType,
                    UMTS_SM_CAUSE_REGUALR_DEACTIVATION);
            }
            else if (appInfo->cmData.smData->smState ==
                UMTS_SM_MS_PDP_REJECTED &&
                appInfo->lastActiveTime +
                UMTS_DEFAULT_FLOW_RESTART_INTERVAL <
                curTime)
            {
                // remove it from the pdpInfo List
                // to make those rejected(deu to QoS) applicaiton
                // retry
                UmtsLayer3UeAppInfoFinalize(node, ueL3, appInfo);
                (*it) = NULL;
            }
        }
    }
    ueL3->appInfoList->remove(NULL);
    MESSAGE_Send(node, msg, UMTS_DEFAULT_FLOW_TIMEOUT_INTERVAL);

    return TRUE;
}

//
// RRC notification function interface for NAS stratum
//
// /**
// FUNCTION   :: UmtsLayer3UeRrcConnEstInd
// LAYER      :: Layer3 RRC
// PURPOSE    :: Notify MM/GMM whether the RRC conn. has been established
// PARAMETERS ::
// +node      : Node*               : Pointer to node.
// +umtsL3    : UmtsLayer3Data *    : Pointer to UMTS Layer3 data
// +BOOL      : success             : Whether RRC connection has
//                                    been established or not
// RETURN     :: NULL
// **/
static
void UmtsLayer3UeRrcConnEstInd(Node* node,
                               UmtsLayer3Data *umtsL3,
                               BOOL  success)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*) umtsL3->dataPtr;

    if (ueL3->mmData->mainState ==
        UMTS_MM_MS_WAIT_FOR_RR_CONNECTION_LOCATION_UPDATING)
    {
        if (success)
        {
            UmtsMmUeSendLocationUpdateRequest(
                    node,
                    umtsL3,
                    ueL3->mmData->nextLocUpType);

            // update stat
            ueL3->stat.numLocUpReqSent ++;
        }
        else
        {
            ueL3->mmData->mainState = UMTS_MM_MS_MM_IDLE;
            ueL3->mmData->idleSubState = UMTS_MM_MS_IDLE_NORMAL_SERVICE;
        }
    }
    else if (ueL3->mmData->mainState ==
                UMTS_MM_MS_WAIT_FOR_RR_ACTIVE)
    {
        if (success)
        {
            UmtsLayer3UeSendPagingResp(node, umtsL3, ueL3);
            ueL3->mmData->mainState = UMTS_MM_MS_WAIT_FOR_NETWORK_COMMAND;
            ERROR_Assert(ueL3->mmData->T3240 == NULL,
                            "Inconsistent MM state");
            ueL3->mmData->T3240 = UmtsLayer3SetTimer(
                            node,
                            umtsL3,
                            UMTS_LAYER3_TIMER_T3240,
                            UMTS_LAYER3_TIMER_T3240_VAL,
                            NULL,
                            NULL,
                            0);
        }
        else
        {
            ueL3->mmData->mainState = UMTS_MM_MS_MM_IDLE;
            ueL3->mmData->idleSubState = UMTS_MM_MS_IDLE_NORMAL_SERVICE;
        }
    }
    else if (ueL3->mmData->mainState ==
                UMTS_MM_MS_WAIT_FOR_RR_CONNECTION_MM_CONNECTION)
    {
        if (success)
        {
            UmtsLayer3PD pd = ueL3->mmData->rqtingMmConns->front()->pd;
            unsigned char ti = ueL3->mmData->rqtingMmConns->front()->ti;
            UmtsLayer3UeSendCmServReq(node, umtsL3, ueL3->mmData, pd, ti);
            ueL3->mmData->mainState =
                UMTS_MM_MS_WAIT_FOR_OUTGOING_MM_CONNECTION;
        }
        else
        {
            while (!ueL3->mmData->rqtingMmConns->empty())
            {
                UmtsLayer3PD pd = ueL3->mmData->rqtingMmConns->front()->pd;
                unsigned char ti = ueL3->mmData->rqtingMmConns->front()->ti;
                delete ueL3->mmData->rqtingMmConns->front();
                ueL3->mmData->rqtingMmConns->pop_front();

                UmtsLayer3UeMmConnEstInd(node, umtsL3, pd, ti, FALSE);
            }
            ueL3->mmData->mainState = UMTS_MM_MS_MM_IDLE;
            ueL3->mmData->idleSubState = UMTS_MM_MS_IDLE_NORMAL_SERVICE;
        }
    }
    else if (ueL3->gmmData->mainState == UMTS_GMM_MS_DEREGISTERED &&
        ueL3->gmmData->deRegSubState ==
                UMTS_GMM_MS_DEREGISTERED_ATTACH_NEEDED)
    {
        if (success)
        {
            UmtsAttachType attachType = ueL3->gmmData->nextAttachType;
            // init attach attempt counter
            ueL3->gmmData->attachAtmpCount = 0;

            // send the attach request
            UmtsGMmUeSendAttachRequest(
                node, umtsL3, attachType);
            ueL3->stat.numAttachReqSent ++;

            // start timer T3310
            if (ueL3->gmmData->T3310)
            {
                MESSAGE_CancelSelfMsg(node, ueL3->gmmData->T3310);
                ueL3->gmmData->T3310 = NULL;
            }

            ueL3->gmmData->T3310 =
                UmtsLayer3SetTimer(
                    node, umtsL3, UMTS_LAYER3_TIMER_T3310,
                    UMTS_LAYER3_TIMER_T3310_VAL, NULL,
                    (void*)&attachType, sizeof(UmtsAttachType));

            // GMM enter registed initiated state
            UmtsGMmUeSetGMmState(ueL3, UMTS_GMM_MS_REGISTERED_INITIATED);
        }
        else
        {
            // do nothing
        }
    }
    else if (ueL3->gmmData->mainState ==
                UMTS_GMM_MS_SERVICE_REQUEST_INITIATED
             && ueL3->gmmData->servReqWaitForRrc == TRUE)
    {
        if (success)
        {
            UmtsLayer3UeSendServiceRequest(
                node, umtsL3, ueL3,
                ueL3->gmmData->serviceTypeRequested,
                ueL3->gmmData->servReqTransId);

            // update stat
            ueL3->stat.numServiceReqSent ++;

            ueL3->gmmData->pmmState = UMTS_PMM_CONNECTED;
        }
        else
        {
            ueL3->gmmData->mainState = UMTS_GMM_MS_NULL;
        }
    }
}

// FUNCTION   :: UmtsLayer3UeRrcConnRelInd
// LAYER      :: Layer3 RRC
// PURPOSE    :: Notify MM/GMM the RRC connection has been released
// PARAMETERS ::
// + node      : Node*               : Pointer to node.
// + umtsL3    : UmtsLayer3Data *    : Pointer to UMTS Layer3 data
// RETURN     :: NULL
// **/
static
void UmtsLayer3UeRrcConnRelInd(Node* node,
                               UmtsLayer3Data *umtsL3)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*) umtsL3->dataPtr;

    UmtsLayer3UeSigConnRelInd(node,
                              umtsL3,
                              ueL3,
                              UMTS_LAYER3_CN_DOMAIN_PS);
    UmtsLayer3UeSigConnRelInd(node,
                              umtsL3,
                              ueL3,
                              UMTS_LAYER3_CN_DOMAIN_CS);

    //reset GMM data
    //ueL3->gmmData->updateState = UMTS_GMM_NOT_UPDATED;
    ueL3->gmmData->mainState = UMTS_GMM_MS_DEREGISTERED;
    ueL3->gmmData->pmmState = UMTS_PMM_DETACHED;

    //reset MM data
    ueL3->mmData->mainState = UMTS_MM_MS_MM_IDLE;
    ueL3->mmData->idleSubState = UMTS_MM_MS_IDLE_PLMN_SEARCH;
    //ueL3->mmData->updateState = UMTS_MM_NOT_UPDATED;
}

// /**
// FUNCTION   :: UmtsLayer3UeMmConnEstInd
// LAYER      :: Layer3 RRC
// PURPOSE    :: Notify CM whether the MM connection has been established
// PARAMETERS ::
// +node      : Node*               : Pointer to node.
// +umtsL3    : UmtsLayer3Data *    : Pointer to UMTS Layer3 data
// +pd        : UmtsLayer3PD        : Protocol discriminator
// +success   : BOOL                : Whether RRC connection has
//                                    been established or not
// RETURN     :: NULL
// **/
static
void UmtsLayer3UeMmConnEstInd(Node* node,
                              UmtsLayer3Data *umtsL3,
                              UmtsLayer3PD pd,
                              unsigned char ti,
                              BOOL  success)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*)(umtsL3->dataPtr);
    UmtsLayer3UeAppInfo* ueFlow = UmtsLayer3UeFindAppInfoFromList(
                                    node,
                                    ueL3,
                                    UMTS_FLOW_MOBILE_ORIGINATED,
                                    ti);
    if (ueFlow)
    {
        if (pd == UMTS_LAYER3_PD_CC)
        {
            UmtsCcMsData* ueCc = ueFlow->cmData.ccData;
            if (success)
            {
                if (ueCc->state == UMTS_CC_MS_MM_CONNECTION_PENDING)
                {
                    UmtsLayer3UeSendCallSetup(node,
                                              umtsL3,
                                              ti,
                                              ueCc);
                    ueCc->state = UMTS_CC_MS_CALL_INITIATED;
                }
            }
            else
            {
                if (ueCc->state == UMTS_CC_MS_MM_CONNECTION_PENDING)
                {
                    UmtsLayer3UeNotifyAppCallReject(
                        node,
                        umtsL3,
                        ueFlow,
                        APP_UMTS_CALL_REJECT_CAUSE_SYSTEM_BUSY);

                    UmtsLayer3UeRemoveAppInfoFromList(
                        node,
                        ueL3,
                        ueFlow->srcDestType,
                        ueFlow->transId);
                }
            } // if (success)
        } // if (pd == UMTS_LAYER3_PD_CC)
        else
        {
            ERROR_ReportError("Wrong PD for active MM connection.");
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3UeMmConnRelInd
// LAYER      :: Layer3 RRC
// PURPOSE    :: Notify CM an active MM connection is released
// PARAMETERS ::
// + node      : Node*               : Pointer to node.
// + umtsL3    : UmtsLayer3Data *    : Pointer to UMTS Layer3 data
// + pd        : UmtsLayer3PD        : Protocol discriminator
// + BOOL      : success             : Whether RRC connection has
//                                    been established or not
// RETURN     :: NULL
// **/
static
void UmtsLayer3UeMmConnRelInd(Node* node,
                              UmtsLayer3Data *umtsL3,
                              UmtsLayer3PD pd,
                              unsigned char ti)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*)(umtsL3->dataPtr);
    UmtsFlowSrcDestType srcDestType;
    if (ti < UMTS_LAYER3_NETWORK_MOBILE_START)
    {
        srcDestType = UMTS_FLOW_MOBILE_ORIGINATED;
    }
    else
    {
        srcDestType = UMTS_FLOW_NETWORK_ORIGINATED;
    }
    UmtsLayer3UeAppInfo* ueFlow = UmtsLayer3UeFindAppInfoFromList(
                                    node,
                                    ueL3,
                                    srcDestType,
                                    ti);
    if (ueFlow)
    {
        if (pd == UMTS_LAYER3_PD_CC)
        {
            UmtsCcMsData* ueCc = ueFlow->cmData.ccData;
            if (ueCc->state != UMTS_CC_MS_NULL
                && ueCc->state != UMTS_CC_MS_DISCONNECT_REQUEST
                && ueCc->state != UMTS_CC_MS_RELEASE_REQUEST)
            {
                UmtsLayer3UeNotifyAppCallClearing(
                        node,
                        umtsL3,
                        ueFlow,
                        UMTS_CALL_CLEAR_DROP);
            }
        }
        UmtsLayer3UeRemoveAppInfoFromList(
            node,
            ueL3,
            srcDestType,
            ti);
    }
}

// FUNCTION   :: UmtsLayer3UeHandleNasMsg
// LAYER      :: Layer3 RRC
// PURPOSE    :: Notify MM/GMM the arrival of NAS message
//               Note: user do not try to release the buffer containing
//               the NAS message. If need to store the message, keep a
//               copy, since the message will be released after the function
//               returns
// PARAMETERS ::
// + node      : Node*               : Pointer to node.
// + umtsL3    : UmtsLayer3Data *    : Pointer to UMTS Layer3 data
// + nasMsg    : char*               : Pointer to the NAS message content
// + nasMsgSize: int                 : NAS message size
// + domain    : UmtsLayer3CnDomainId: Domain indicator
// RETURN     :: NULL
// **/
static
void UmtsLayer3UeHandleNasMsg(Node* node,
                              UmtsLayer3Data *umtsL3,
                              char* msg,
                              int   msgSize,
                              UmtsLayer3CnDomainId domain)
{
    UmtsLayer3Header l3Header;
    memcpy(&l3Header, msg, sizeof(UmtsLayer3Header));

    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*) umtsL3->dataPtr;
    if (l3Header.pd == UMTS_LAYER3_PD_CC
       /* || l3Header.pd == UMTS_LAYER3_PD_GCC
        || l3Header.pd == UMTS_LAYER3_PD_BCC
        || l3Header.pd == UMTS_LAYER3_PD_SM
        || l3Header.pd == UMTS_LAYER3_PD_LS */)
    {
        UmtsMmMsData* ueMm = ueL3->mmData;
        UmtsLayer3MmConn cmpMmConn =
            UmtsLayer3MmConn((UmtsLayer3PD)l3Header.pd,
                              l3Header.tiSpd);
        if (ueMm->actMmConns->end() ==
                find_if(ueMm->actMmConns->begin(),
                        ueMm->actMmConns->end(),
                        bind2nd(UmtsPointedItemEqual<UmtsLayer3MmConn>(),
                                &cmpMmConn)))
        {
            ueMm->actMmConns->push_back(new UmtsLayer3MmConn(cmpMmConn));
        }
        if (ueMm->mainState !=
            UMTS_MM_MS_WAIT_FOR_ADDITIONAL_OUTGOING_MM_CONNECTION
            && ueMm->mainState != UMTS_MM_MS_MM_CONNECTION_ACTIVE)
        {
            ueMm->mainState = UMTS_MM_MS_MM_CONNECTION_ACTIVE;
        }
        if (ueMm->T3240)
        {
            MESSAGE_CancelSelfMsg(node, ueMm->T3240);
            ueMm->T3240 = NULL;
        }
    }

    switch (l3Header.pd)
    {
        case UMTS_LAYER3_PD_CC:
        {
            UmtsLayer3UeHandleCcMsg(
                node,
                umtsL3,
                (UmtsCcMessageType)l3Header.msgType,
                (char)l3Header.tiSpd,
                msg + sizeof(UmtsLayer3Header),
                msgSize - sizeof(UmtsLayer3Header));
            break;
        }
        case UMTS_LAYER3_PD_MM:
        {
            UmtsLayer3UeHandleMmMsg(
                node,
                umtsL3,
                (UmtsMmMessageType)l3Header.msgType,
                (char)l3Header.tiSpd,
                msg + sizeof(UmtsLayer3Header),
                msgSize - sizeof(UmtsLayer3Header));
            break;
        }
        case UMTS_LAYER3_PD_GMM:
        {
            UmtsLayer3UeHandleGMmMsg(
                node,
                umtsL3,
                (UmtsGMmMessageType)l3Header.msgType,
                (char)l3Header.tiSpd,
                msg + sizeof(UmtsLayer3Header),
                msgSize - sizeof(UmtsLayer3Header));
            break;
        }
        case UMTS_LAYER3_PD_SM:
        {
            UmtsLayer3UeHandleSmMsg(
                node,
                umtsL3,
                (UmtsSmMessageType)l3Header.msgType,
                (char)l3Header.tiSpd,
                msg + sizeof(UmtsLayer3Header),
                msgSize - sizeof(UmtsLayer3Header));

            break;
        }
        default:
        {
            char errStr[MAX_STRING_LENGTH];
            sprintf(errStr,
                    "UMTS Layer3: UE: %d receivers a layer3 "
                    "signalling msg with unknown type pd %d",
                    node->nodeId,
                    l3Header.pd);
            ERROR_ReportWarning(errStr);
            break;
        }
    }
}

#if 0
// FUNCTION   :: UmtsLayer3UeIndPagingToNas
// LAYER      :: Layer3 RRC
// PURPOSE    :: Notify NAS stratum incoming Paging message
// PARAMETERS ::
// +node      : Node*               : Pointer to node.
// +umtsL3    : UmtsLayer3Data *    : Pointer to UMTS Layer3 data
// +domain    : UmtsLayer3CnDomainId: Domain indicator
// +cause     : UmtsPagingCause     : Paging cause
// RETURN     :: NULL
// **/
static
void UmtsLayer3UeIndPagingToNas(
    Node* node,
    UmtsLayer3Data *umtsL3,
    UmtsLayer3CnDomainId domain,
    UmtsPagingCause cause)
{
    if (domain == UMTS_LAYER3_CN_DOMAIN_CS)
    {
        //TODO: call CC procedure to handle paging
    }
    else
    {
        //TODO: call SM procedure to handle paging
    }
}
#endif

//
// handle Timer Msg
//

// /**
// FUNCTION   :: UmtsLayer3UeHandleTimerMsg
// LAYER      :: Layer 3
// PURPOSE    :: Handle a timer expired msg
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + ueL3      : UmtsLayer3Ue *   : UE data
// + msg       : Message*         : Message containing the packet
// RETURN     :: BOOL : msg need ot be reused or not
// **/
static
BOOL UmtsLayer3UeHandleTimerMsg(Node *node,
                                UmtsLayer3Data *umtsL3,
                                UmtsLayer3Ue *ueL3,
                                Message *msg)
{
    UmtsLayer3TimerType timerType;
    UmtsLayer3Timer *timerInfo;
    BOOL msgReuse = FALSE;

    // get timer type first
    timerInfo = (UmtsLayer3Timer *) MESSAGE_ReturnInfo(msg);
    timerType = timerInfo->timerType;

    switch (timerType)
    {
        case UMTS_LAYER3_TIMER_POWERON:
        {
            // the UE is powered on
            UmtsLayer3UePowerOn(node, umtsL3);

            break;
        }
        case UMTS_LAYER3_TIMER_POWEROFF:
        {
            // the UE is powered off
            //UmtsLayer3UePowerOff(node, umtsL3);

            break;
        }
        //RRC timers
        case UMTS_LAYER3_TIMER_T300:
        {
            msgReuse = UmtsLayer3UeHandleTimerT300(
                           node,
                           umtsL3,
                           ueL3,
                           msg);
            break;
        }
        case UMTS_LAYER3_TIMER_T308:
        {
            msgReuse = UmtsLayer3UeHandleTimerT308(
                           node,
                           umtsL3,
                           ueL3,
                           msg);
            break;
        }
        case UMTS_LAYER3_TIMER_RRCCONNWAITTIME:
        {
            msgReuse = UmtsLayer3UeHandleTimerRrcConnReqWaitTime(
                            node,
                            umtsL3,
                            ueL3,
                            msg);
            break;
        }
        //MM/GMM timers
        case UMTS_LAYER3_TIMER_T3212:
        {
            msgReuse = UmtsMmHandleTimerT3212(node, umtsL3, ueL3, msg);
            break;
        }
        case UMTS_LAYER3_TIMER_T3230:
        {
            UmtsMmHandleTimerT3230(node, umtsL3, ueL3, msg);
            break;
        }
        case UMTS_LAYER3_TIMER_T3240:
        {
            UmtsMmHandleTimerT3240(node, umtsL3, ueL3, msg);
            break;
        }
        case UMTS_LAYER3_TIMER_T3310:
        {
            msgReuse = UmtsGMmHandleTimerT3310(node, umtsL3, ueL3, msg);

            break;
        }
        case UMTS_LAYER3_TIMER_T3317:
        {
            msgReuse = UmtsGMmHandleTimerT3317(node, umtsL3, ueL3, msg);

            break;
        }
        case UMTS_LAYER3_TIMER_T3319:
        {
            //ueL3->gmmData->T3319 = NULL;

            break;
        }
        //CC timers
        case UMTS_LAYER3_TIMER_CC_T303:
        {
            UmtsLayer3UeHandleCcT303(node, umtsL3, ueL3, msg);
        }
        case UMTS_LAYER3_TIMER_CC_T310:
        {
            UmtsLayer3UeHandleCcT310(node, umtsL3, ueL3, msg);
        }
        case UMTS_LAYER3_TIMER_CC_T313:
        {
            UmtsLayer3UeHandleCcT313(node, umtsL3, ueL3, msg);
        }
        case UMTS_LAYER3_TIMER_CC_T305:
        {
            UmtsLayer3UeHandleCcT305(node, umtsL3, ueL3, msg);
        }
        case UMTS_LAYER3_TIMER_CC_T308:
        {
            UmtsLayer3UeHandleCcT308(node, umtsL3, ueL3, msg);
        }
        //SM timers
        case UMTS_LAYER3_TIMER_T3380:
        {
            msgReuse = UmtsLayer3UeSmHandleTimerT3380(
                           node, umtsL3, ueL3, msg);

            break;
        }
        case UMTS_LAYER3_TIMER_T3390:
        {
            msgReuse = UmtsLayer3UeSmHandleTimerT3390(
                           node, umtsL3, ueL3, msg);

            break;
        }
        case UMTS_LAYER3_TIMER_FLOW_TIMEOUT:
        {
            msgReuse = UmtsLayer3UeHandleAppTimeout(
                           node, umtsL3, ueL3, msg);

            break;
        }
        case UMTS_LAYER3_TIMER_CPICHCHECK:
        {
            msgReuse = UmtsLayer3UeHandleTimerCpichCheck(
                           node, umtsL3, ueL3, msg);

            break;
        }
        default:
        {
            char errStr[MAX_STRING_LENGTH];
            sprintf(errStr,
                    "UMTS Layer3: Node%d(UE) receivers a timer"
                    "event with unknown type %d",
                    node->nodeId,
                    timerType);
            ERROR_ReportWarning(errStr);

            break;
        }
    }

    return msgReuse;
}

///////
// Hanlde Packet
//////////
// /**
// FUNCTION   :: UmtsLayer3UeHandleCsPacketFromUpper
// LAYER      :: Layer 3
// PURPOSE    :: Handle a timer expired msg
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + ueL3      : UmtsLayer3Ue *   : UE data
// + msg       : Message*         : Message containing the packet
// RETURN     :: BOOL : msg need ot be reused or not
// **/
static
void UmtsLayer3UeHandleCsPacketFromUpper(Node *node,
                                UmtsLayer3Data *umtsL3,
                                UmtsLayer3Ue *ueL3,
                                UmtsLayer3UeAppInfo* appInfo,
                                Message *msg)
{
    // get the application CC status
    // if it is able to send the packet out, put the packet to
    // RLC
    // otherwise put it to the applicaiton's queue
    if (appInfo->cmData.ccData->state == UMTS_CC_MS_NULL)
    {
        // to see if any MM exsit
        // put the msg in queue
        appInfo->inPacketBuf->push(msg);

        // update stat
        ueL3->stat.numCsDataPacketFromUpperEnQueue ++;
        ueL3->stat.numCsDataPacketFromUpper ++;
        // init MM connection
    }else
    {
        // do nothing
    }
}

// /**
// FUNCTION   :: UmtsLayer3UeHandlePsPacketFromUpper
// LAYER      :: Layer 3
// PURPOSE    :: Handle a timer expired msg
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + ueL3      : UmtsLayer3Ue *   : UE data
// + appInfo   : UmtsLayer3UeAppInfo* : Application Info
// + msg       : Message*         : Message containing the packet
// RETURN     :: BOOL : msg need ot be reused or not
// **/
static
void UmtsLayer3UeHandlePsPacketFromUpper(Node *node,
                                UmtsLayer3Data *umtsL3,
                                UmtsLayer3Ue *ueL3,
                                UmtsLayer3UeAppInfo* appInfo,
                                Message *msg)
{
    // get the application SM status
    // if it is able to send the packet out, put the packet to
    // RLC
    // otherwise put it to the applicaiton's queue

    // update stat
    ueL3->stat.numPsDataPacketFromUpper ++;

    //Set message protocol as CELLULAR so it can not mistaken as IP packet
    //within cellular network (may not be neccessary since RNC already take
    //care of this).
    MESSAGE_SetLayer(msg, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR);

    // if the flow has been rejected, just free the packet
    if (ueL3->rrcData->state == UMTS_RRC_STATE_IDLE
        && ueL3->rrcData->subState
            < UMTS_RRC_SUBSTATE_SUITABLE_CELL_SELECTED)
    {
        MESSAGE_Free(node, msg);
        ueL3->stat.numPsPktDroppedNoResource++;
    }
    else if (ueL3->gmmData->mainState == UMTS_GMM_MS_DEREGISTERED)
    {
        // not attached yet
        UmtsLayer3UeEnquePsPktFromUpper(node, ueL3, appInfo,msg);
        UmtsLayer3UeInitGmmAttachProc(
            node, umtsL3, ueL3, TRUE, FALSE);
    }
    else if (ueL3->gmmData->mainState == UMTS_GMM_MS_REGISTERED)
    {
        if (ueL3->gmmData->pmmState == UMTS_PMM_IDLE &&
            !ueL3->gmmData->AttCompWaitRelInd)
        {
            UmtsLayer3UeEnquePsPktFromUpper(node, ueL3, appInfo, msg);

            // idle, no ps signalling,
            // and no other onging service request ongoing
            // no matter PDP is active or inactive, the first thing
            // is to build the ps signalling
            unsigned char serviceType;
            if (appInfo->cmData.smData->smState ==
                UMTS_SM_MS_PDP_INACTIVE)
            {
                // activate PDP request need to be sent
                // after service request
                serviceType = 0x00; // 00000000 signalling
                appInfo->cmData.smData->needSendActivateReq = TRUE;
            }
            else
            {
                serviceType = 0x01; // data
            }
            // send service request PDP context
            UmtsRrcEstCause estCause; // TODO get appInfo's class
            estCause = UMTS_RRC_ESTCAUSE_ORIGINATING_BACKGROUND;
            unsigned char transId = appInfo->transId;
            UmtsLayer3UeInitServiceRequest(
               node, umtsL3, ueL3,
               estCause,
               serviceType, transId);
        }
        else
        {
            UmtsSmMsState smState = appInfo->cmData.smData->smState;
            if (smState == UMTS_SM_MS_PDP_INACTIVE)
            {
                UmtsLayer3UeEnquePsPktFromUpper(node, ueL3, appInfo, msg);
                UmtsLayer3UeSmInitActivatePDPContextRequest(
                    node,
                    umtsL3,
                    ueL3,
                    appInfo);
            }
            else if (smState == UMTS_SM_MS_PDP_ACTIVE)
            {
                UmtsLayer3UeSendPacketToLayer2(node,
                                       umtsL3->interfaceIndex,
                                       appInfo->rbId,
                                       msg);

                // update stat
                ueL3->stat.numPsDataPacketSentToL2 ++;
            }
            else if (smState == UMTS_SM_MS_PDP_REJECTED
                     || smState == UMTS_SM_MS_PDP_INACTIVE_PENDING)
            {
                MESSAGE_Free(node, msg);
                ueL3->stat.numPsPktDroppedNoResource ++;
            }
            else
            {
                UmtsLayer3UeEnquePsPktFromUpper(node, ueL3, appInfo, msg);
            }
        }
    }
    else if (ueL3->gmmData->mainState ==
                UMTS_GMM_MS_SERVICE_REQUEST_INITIATED)
    {
        if (ueL3->gmmData->pmmState == UMTS_PMM_IDLE)
        {
            // since there is a ongoing service request on going
            // set state so the PDP request can be sent after service accept
            UmtsLayer3UeEnquePsPktFromUpper(node, ueL3, appInfo, msg);

            if (appInfo->cmData.smData->smState == UMTS_SM_MS_PDP_INACTIVE)
            {
                appInfo->cmData.smData->needSendActivateReq = TRUE;
            }
        }
        else
        {
            UmtsSmMsState smState = appInfo->cmData.smData->smState;
            if (smState == UMTS_SM_MS_PDP_INACTIVE)
            {
                UmtsLayer3UeEnquePsPktFromUpper(node, ueL3, appInfo, msg);
                UmtsLayer3UeSmInitActivatePDPContextRequest(
                    node,
                    umtsL3,
                    ueL3,
                    appInfo);
            }
            else if (smState == UMTS_SM_MS_PDP_ACTIVE)
            {
                UmtsLayer3UeSendPacketToLayer2(node,
                                       umtsL3->interfaceIndex,
                                       appInfo->rbId,
                                       msg);
                ueL3->stat.numPsDataPacketSentToL2 ++;
            }
            else if (smState == UMTS_SM_MS_PDP_REJECTED
                     || smState == UMTS_SM_MS_PDP_INACTIVE_PENDING)
            {
                MESSAGE_Free(node, msg);
                ueL3->stat.numPsPktDroppedNoResource ++;
            }
            else
            {
                UmtsLayer3UeEnquePsPktFromUpper(node, ueL3, appInfo, msg);
            }
        }
    }
    else
    {
        UmtsLayer3UeEnquePsPktFromUpper(node, ueL3, appInfo, msg);
    }
}

//--------------------------------------------------------------------------
//  Key API functions
//--------------------------------------------------------------------------
// /**
// FUNCTION   :: UmtsLayer3UeReportAmRlcError
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
void UmtsLayer3UeReportAmRlcError(Node *node,
                                  UmtsLayer3Data* umtsL3,
                                  NodeId ueId,
                                  char rbId)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*) umtsL3->dataPtr;
    UmtsRrcUeData* ueRrc = ueL3->rrcData;

    if (ueRrc->state != UMTS_RRC_STATE_CONNECTED)
    {
        // if multiple RB detects the error at the same time
        // this will prevent the duplicate of RL
        // release procedure
        return;
    }

    if (DEBUG_RR)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n\t is notified of an AM RLC error in RB: %d\n",
            rbId);
    }
    if (rbId <= UMTS_SRB4_ID && rbId > UMTS_SRB1_ID)
    {
        ueRrc->amRlcErrRb234 = TRUE;
    }
    else
    {
        ueRrc->amRlcErrRb5up = TRUE;
    }

    //release RRC connection abnormally
    if (DEBUG_RR_RELEASE)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n\t Release RRC_CONNECTION_RELEASE due to AM RLC error\n");
    }
    UmtsLayer3UeReleaseDedctRsrc(node, umtsL3);
    UmtsLayer3UeEnterIdle(node, umtsL3, ueL3);
    UmtsLayer3UeUpdateStatAfterFailure(node, umtsL3, ueL3);
    UmtsLayer3UeRrcConnRelInd(node, umtsL3);

    // do cell reselection
    UmtsLayer3UeSearchNewCell(node, umtsL3, ueL3);
}

//  /**
// FUNCITON   :: UmtsLayer3UeHandleInterLayerCommand
// LAYER      :: UMTS L3 at UE
// PURPOSE    :: Handle Interlayer command
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + cmdType          : UmtsInterlayerCommandType : command type
// + interfaceIdnex   : UInt32            : interface index of UMTS
// + cmd              : void*             : cmd to handle
// RETURN     :: void : NULL
void UmtsLayer3UeHandleInterLayerCommand(
            Node* node,
            UInt32 interfaceIndex,
            UmtsInterlayerCommandType cmdType,
            void* cmd)
{
    UmtsLayer3Data* umtsL3;
    UmtsLayer3Ue *ueL3;

    umtsL3 = UmtsLayer3UeGetData(node);
    ueL3 = (UmtsLayer3Ue*) umtsL3->dataPtr;

    switch(cmdType)
    {
        case UMTS_CPHY_MEASUREMENT_IND:
        {
           UmtsLayer3UeHandleMeasuremnetInd
               (node, umtsL3, ueL3, cmd);

           break;
        }
        case UMTS_CMAC_STATUS_IND:
        {
            // TODO
            break;
        }
        default:
        {
            // do nothing
        }
    }
}
// /**
// FUNCTION   :: UmtsLayer3UeInitDynamicStats
// LAYER      :: Layer3
// PURPOSE    :: Initiate dynamic statistics
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + interfaceIdnex   : UInt32 : interface index of UMTS
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + ueL3      : UmtsLayer3Ue *   : POinter to PHY UMTS UE data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3UeInitDynamicStats(Node *node,
                                  UInt32 interfaceIndex,
                                  UmtsLayer3Data *umtsL3,
                                  UmtsLayer3Ue *ueL3)
{
    if (!node->guiOption)
    {
        return;
    }

    // CPICH RSCP
    ueL3->stat.priNodeBCpichRscpGuiId =
        GUI_DefineMetric("UMTS UE: Primary NodeB CPICH RSCP (dBm)",
                         node->nodeId,
                         GUI_PHY_LAYER,//GUI_NETWORK_LAYER,
                         interfaceIndex,
                         GUI_DOUBLE_TYPE,
                         GUI_AVERAGE_METRIC);

    // CPICH EcNo
    ueL3->stat.priNodeBCpichEcNoGuiId =
        GUI_DefineMetric("UMTS UE: Primary NodeB CPICH Ec/No (dB)",
                         node->nodeId,
                         GUI_PHY_LAYER,//GUI_NETWORK_LAYER,
                         interfaceIndex,
                         GUI_DOUBLE_TYPE,
                         GUI_AVERAGE_METRIC);

    // CPICH BER
    ueL3->stat.priNodeBCpichBerGuiId =
        GUI_DefineMetric("UMTS UE: Primary NodeB CPICH BER",
                         node->nodeId,
                         GUI_PHY_LAYER,//GUI_NETWORK_LAYER,
                         interfaceIndex,
                         GUI_DOUBLE_TYPE,
                         GUI_AVERAGE_METRIC);

    // Active Set Size
    ueL3->stat.activeSetSizeGuiId =
        GUI_DefineMetric("UMTS UE: Size of Active Set",
                         node->nodeId,
                         GUI_NETWORK_LAYER,
                         interfaceIndex,
                         GUI_INTEGER_TYPE,
                         GUI_CUMULATIVE_METRIC);
}

// /**
// FUNCTION   :: UmtsLayer3UeInit
// LAYER      :: Layer 3
// PURPOSE    :: Initialize UE data at UMTS layer 3 data.
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + nodeInput : const NodeInput* : Pointer to node input.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// RETURN     :: void : NULL
// **/
void UmtsLayer3UeInit(Node *node,
                      const NodeInput *nodeInput,
                      UmtsLayer3Data *umtsL3)
{
    UmtsLayer3Ue *ueL3;
    clocktype delay;

    BOOL retVal = FALSE;
    clocktype uePowerOnTime = 0;

    // initialize the basic UMTS layer3 data
    ueL3 = (UmtsLayer3Ue *) MEM_malloc(sizeof(UmtsLayer3Ue));
    ERROR_Assert(ueL3 != NULL, "UMTS: Out of memory!");
    memset(ueL3, 0, sizeof(UmtsLayer3Ue));

    umtsL3->dataPtr = (void *) ueL3;

    ueL3->imsi = new CellularIMSI(node->nodeId);
    ueL3->locRoutAreaId = UMTS_INVALID_REG_AREA_ID;
    ueL3->plmnType = PLMN_GSM_MAP;
    ueL3->duplexMode = UmtsGetDuplexMode(node, umtsL3->interfaceIndex);

    // HSDPA capability
    ueL3->hsdpaEnabled = UmtsLayer3GetHsdpaCapability(
                                node, umtsL3->interfaceIndex);

    // read configuration parameters
    UmtsLayer3UeInitParameter(node, nodeInput, ueL3);

    if (DEBUG_PARAMETER)
    {
        UmtsLayer3UePrintParameter(node, ueL3);
    }

    // inti MM data
    UmtsLayer3UeInitMm(node, nodeInput, ueL3);

    // init GMM data
    UmtsLayer3UeInitGMm(node, nodeInput, umtsL3, ueL3);

    //Initialize UE RRC sublayer
    UmtsLayer3UeInitRrc(node, nodeInput, ueL3);

    // init dynamic statistics
    UmtsLayer3UeInitDynamicStats(
        node, umtsL3->interfaceIndex, umtsL3, ueL3);

    ueL3->ueNodeInfo = new std::list <UmtsLayer3UeNodebInfo*>;

    ueL3->appInfoList = new std::list <UmtsLayer3UeAppInfo*>;
    //ueL3->transIdCount = UMTS_LAYER3_UE_MOBILE_START;

    ueL3->nsapiBitSet = new std::bitset<UMTS_MAX_NASPI>;
    ueL3->transIdBitSet = new std::bitset<UMTS_MAX_TRANS_MOBILE>;

    // the following should be moved to the place when cell is selected
    UmtsLayer3UeConfigCommonRadioBearer(node, umtsL3);

    // User can configure the power on time of UE.
    IO_ReadTime(node->nodeId,
                ANY_ADDRESS,
                nodeInput,
                "UMTS-UE-POWER-ON-TIME",
                &retVal,
                &uePowerOnTime);

        delay = uePowerOnTime + 
            RANDOM_nrand(umtsL3->seed) % UMTS_LAYER3_UE_START_JITTER;

    // set a timer to poweron the UE
    UmtsLayer3SetTimer(node,
                       umtsL3,
                       UMTS_LAYER3_TIMER_POWERON,
                       delay,
                       NULL);

    // set a Timer to check the out of status PS app
    UmtsLayer3SetTimer(
           node,
           umtsL3,
           UMTS_LAYER3_TIMER_FLOW_TIMEOUT,
           UMTS_DEFAULT_FLOW_TIMEOUT_INTERVAL,
           NULL);

    //Set a Timer to remove cell out of range from monitored set
    UmtsLayer3SetTimer(
           node,
           umtsL3,
           UMTS_LAYER3_TIMER_CPICHCHECK,
           UMTS_LAYER3_TIMER_CPICHCHECK_VAL,
           NULL);
}

// /**
// FUNCTION   :: UmtsLayer3UeLayer
// LAYER      :: Layer 3
// PURPOSE    :: Handle UE specific timers and layer messages.
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + msg       : Message*         : Message for node to interpret
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// RETURN     :: void : NULL
// **/
void UmtsLayer3UeLayer(Node *node, Message *msg, UmtsLayer3Data *umtsL3)
{
    UmtsLayer3Ue *ueL3 = (UmtsLayer3Ue *) (umtsL3->dataPtr);
    BOOL msgReused = FALSE;

    switch((msg->eventType))
    {
        case MSG_NETWORK_CELLULAR_TimerExpired:
        {
            msgReused = UmtsLayer3UeHandleTimerMsg(node, umtsL3, ueL3, msg);
            break;
        }

        case MSG_NETWORK_CELLULAR_FromAppStartCall:
        {
            UmtsLayer3UeHandleStartCallMsg(node, umtsL3, msg);
            break;
        }

        case MSG_NETWORK_CELLULAR_FromAppEndCall:
        {
            UmtsLayer3UeHandleEndCallMsg(node, umtsL3, msg);
            break;
        }

        case MSG_NETWORK_CELLULAR_FromAppCallAnswered:
        {
            UmtsLayer3UeHandleCallAnsweredMsg(node, umtsL3, msg);
            break;
        }

        case MSG_NETWORK_CELLULAR_FromAppPktArrival:
        {
            msgReused = UmtsLayer3UeHandleCallPktArrival(node, umtsL3, msg);
            break;
        }
        case MSG_NETWORK_FromMac:
        {
            char* info = MESSAGE_ReturnInfo(msg);
            int index = 0;
            UmtsInterlayerMsgType type;
            memcpy(&type, info, sizeof(type));
            index += sizeof(type);
            if (type == UMTS_REPORT_AMRLC_ERROR)
            {
                NodeId ueId;
                char rbId;
                memcpy(&ueId, &info[index], sizeof(ueId));
                index += sizeof(ueId);
                rbId = info[index++];

                UmtsLayer3UeReportAmRlcError(node,
                             umtsL3,
                             ueId,
                             rbId);
            }
            else {
                ERROR_ReportWarning("Received unknown message type "
                    "from MAC layer.");
            }
            break;
        }

        default:
        {
            char errStr[MAX_STRING_LENGTH];
            sprintf(errStr,
                    "UMTS Layer3: Node%d(UE) got an event with a "
                    "unknown type (%d)",
                    node->nodeId, MESSAGE_GetEvent(msg));
            ERROR_ReportWarning(errStr);
            break;
        }
    }

    if (!msgReused)
    {
        MESSAGE_Free(node, msg);
    }
}

// /**
// FUNCTION   :: UmtsLayer3UeFinalize
// LAYER      :: Layer 3
// PURPOSE    :: Print UE specific stats and clear protocol variables.
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// RETURN     :: void : NULL
// **/
void UmtsLayer3UeFinalize(Node *node, UmtsLayer3Data *umtsL3)
{
    UmtsLayer3Ue *ueL3 = (UmtsLayer3Ue *) (umtsL3->dataPtr);
    std::list<UmtsLayer3UeAppInfo*>::iterator itApp;
    std::list<UmtsLayer3UeNodebInfo*>::iterator itNodeB;

    if (umtsL3->collectStatistics)
    {
        // print UE specific statistics
        UmtsLayer3UePrintStats(node, umtsL3, ueL3);
    }

    // finalize RRC
    UmtsLayer3UeFinalizeRrc(node, umtsL3, ueL3);

    // Finalize MM
    UmtsLayer3UeFinalizeMm(node, umtsL3, ueL3);

    // Finalize GMM
    UmtsLayer3UeFinalizeGMm(node, umtsL3, ueL3);
    // Finalize app,SM/CC

    // Free each appInfo
    for (itApp = ueL3->appInfoList->begin();
        itApp != ueL3->appInfoList->end();
        itApp ++)
    {
        UmtsLayer3UeAppInfoFinalize(node, ueL3, (*itApp));
        *itApp = NULL;
    }
    ueL3->appInfoList->remove(NULL);
    delete ueL3->appInfoList;

    delete ueL3->nsapiBitSet;
    delete ueL3->transIdBitSet;

    for (itNodeB = ueL3->ueNodeInfo->begin();
        itNodeB != ueL3->ueNodeInfo->end();
        itNodeB ++)
    {
            delete (*itNodeB);
            (*itNodeB) = NULL;
    }
    ueL3->ueNodeInfo->remove(NULL);
    delete ueL3->ueNodeInfo;
    delete ueL3->imsi;

    MEM_free(ueL3);
    umtsL3->dataPtr = NULL;
}

// /**
// FUNCTION   :: UmtsLayer3UeReceivePacketFromMacLayer
// LAYER      :: Layer3
// PURPOSE    :: Handle timers and layer messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface from which the packe
//                                          it is received
// + umtsL3           : UmtsLayer3Data *  : Pointer to UMTS Layer3 data
// + msg              : Message*          : Message for node to interpret.
// + lastHopAddress   : NodeAddress       : Address of the last hop
// RETURN     :: void : NULL
// **/
void UmtsLayer3UeReceivePacketFromMacLayer(
    Node *node,
    int interfaceIndex,
    UmtsLayer3Data* umtsL3,
    Message *msg,
    NodeAddress lastHopAddress)
{
    UmtsLayer3Ue* ueL3 = (UmtsLayer3Ue*) umtsL3->dataPtr;
    UmtsLayer2PktToUpperlayerInfo* info =
        (UmtsLayer2PktToUpperlayerInfo*) MESSAGE_ReturnInfo(msg);

    unsigned int rbId = info->rbId;

    // update stat
    ueL3->stat.numPacketRcvdFromL2 ++;

    if (rbId >= UMTS_RBINRAB_START_ID && rbId < UMTS_BCCH_RB_ID)
    {
//GuiStart
        if ((node->guiOption == TRUE) && rbId > 0 && rbId < 32)
        {
            std::list <UmtsLayer3UeNodebInfo*>::iterator it;
            for (it = ueL3->ueNodeInfo->begin();
                    it != ueL3->ueNodeInfo->end(); ++it)
            {
                if ((*it)->cellStatus == UMTS_CELL_ACTIVE_SET)
                {
                    GUI_Receive((*it)->cellId/16,
                                node->nodeId,
                                GUI_NETWORK_LAYER,
                                GUI_DEFAULT_DATA_TYPE,
                                umtsL3->interfaceIndex,
                                interfaceIndex,
                                node->getNodeTime());
                }
            }
        }
//GuiEnd

        // update Stat
        ueL3->stat.numDataPacketRcvdFromL2 ++;

        char rabId = ueL3->rrcData->rbRabId[rbId - UMTS_RBINRAB_START_ID];
        ERROR_Assert(rabId != UMTS_INVALID_RAB_ID,
            "Cannot find the RAB ID associated with received packet");
        UmtsUeRabInfo* rabInfo = ueL3->rrcData->rabInfos[(int)rabId];
        if (rabInfo->cnDomainId == UMTS_LAYER3_CN_DOMAIN_PS)
        {
            ueL3->stat.numPsDataPacketRcvdFromL2 ++;

            //Set message protocol as IP so it can not
            //mistaken as cellular packet
            //assuming only IPv4 is supported.
            MESSAGE_SetLayer(msg, NETWORK_LAYER, NETWORK_PROTOCOL_IP);
            NetworkIpReceivePacketFromMacLayer(node,
                                               msg,
                                               lastHopAddress,
                                               interfaceIndex);
        }
        else if (rabInfo->cnDomainId == UMTS_LAYER3_CN_DOMAIN_CS)
        {
            ueL3->stat.numCsDataPacketRcvdFromL2 ++;
            //send Pkt to the upper layer
            UmtsLayer3UeForwardCsPktToApp(node, umtsL3, rabId, msg);
        }
    }
    else
    {
        char pd;
        char tiSpd;
        char msgType;

        // update stat
        ueL3->stat.numCtrlPacketRcvdFromL2 ++;

        if (umtsL3->newStats)
        {
            umtsL3->newStats->AddPacketReceivedFromMacDataPoints(
                node,
                msg,
                STAT_Unicast,
                interfaceIndex,
                FALSE); // Control
        }

        UmtsLayer3RemoveHeader(node,
                               msg,
                               &pd,
                               &tiSpd,
                               &msgType);

        switch (pd)
        {
            case UMTS_LAYER3_PD_RR:
            {
                UmtsLayer3UeHandleRrMsg(
                    node,
                    interfaceIndex,
                    msgType,
                    tiSpd,
                    msg);
                break;
            }
            default:
            {
                char errStr[MAX_STRING_LENGTH];
                sprintf(errStr,
                        "UMTS Layer3: Node%d(UE) receivers a msg"
                        "with unknown type pd %d",
                        node->nodeId,
                        pd);
                ERROR_ReportError(errStr);

                MESSAGE_Free(node, msg);

                break;
            }
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3UeHandlePacketFromUpper
// LAYER      :: Layer3
// PURPOSE    :: Handle pakcets from upper
// PARAMETERS ::
// + node              : Node*            : Pointer to node.
// + msg  : Message*: Message to be sent onver the air interface
// + interfaceIndex   : int : Interface from which the packet is received
// + networkType      : int               : network Type
// RETURN     :: void : NULL
// **/
void UmtsLayer3UeHandlePacketFromUpper(
            Node* node,
            Message* msg,
            int interfaceIndex,
            int networkType)
{
    UmtsLayer3Data* umtsL3;
    UmtsLayer3Ue *ueL3;

    umtsL3 = UmtsLayer3UeGetData(node);
    ueL3 = (UmtsLayer3Ue*) umtsL3->dataPtr;

    const char* payload;
    UmtsLayer3FlowClassifier appClsInfo;
    TosType tos = 0;
    TraceProtocolType appType;

    UmtsLayer3UeAppInfo* appInfo = NULL;
    BOOL  newApp = FALSE;

    // update stat
    ueL3->stat.numDataPacketFromUpper++;

    // 1. build the callisifer from the msg

    // retrive information from the packet headers
    payload = MESSAGE_ReturnPacket(msg);
    memset(&appClsInfo, 0, sizeof(UmtsLayer3FlowClassifier));

    // build the classifier info struct
    UmtsLayer3BuildFlowClassifierInfo(
            node,
            interfaceIndex,
            networkType,
            msg,
            &tos,
            &appClsInfo,
            &payload);


    // check packet size or other QoS paramenters
    if (!UmtsLayer3CheckFlowQoSParam(
            node, interfaceIndex,
            (TraceProtocolType) msg->originatingProtocol,
            tos, &payload))
    {
        ueL3->stat.numDataPacketDroppedUnsupportedFormat ++;
        MESSAGE_Free(node, msg);

        return;
    }

    // too see if the corrsponding appInfo has been create or not
    appInfo = UmtsLayer3UeFindAppInfoFromList(node,
                                              ueL3,
                                              &appClsInfo);

    // if the it is new app, create new application info
    if (appInfo == NULL)
    {
        unsigned char transId;
        // create a new applicaiton info

        //transId = ueL3->transIdCount;
        //ueL3->transIdCount = (ueL3->transIdCount + 1) %
        //                      (UMTS_LAYER3_UE_MOBILE_END +  1);

        int pos = UmtsGetPosForNextZeroBit<UMTS_MAX_TRANS_MOBILE>
                                          (*(ueL3->transIdBitSet), 0);

        if (pos == -1)
        {
            MESSAGE_Free(node, msg);

            ERROR_ReportWarning(
                "Too many active Transactions, message dropped");
            // TODO: notify applicaiton

            return;
        }
        else
        {
            transId = (unsigned char)pos;
        }
        appInfo = UmtsLayer3UeAddAppInfoToList(node,
                                               ueL3,
                                               UMTS_FLOW_MOBILE_ORIGINATED,
                                               UMTS_LAYER3_CN_DOMAIN_PS,
                                               transId,
                                               &appClsInfo);
        if (!appInfo)
        {
            MESSAGE_Free(node, msg);

            ERROR_ReportWarning("fail to create appInfo, message dropped");
            // TODO: notify applicaiton

            return;
        }
        else
        {
            //UmtsSetBit(*(ueL3->transIdBitSet), transId);
            ueL3->transIdBitSet->set(transId);
            if (appInfo->classifierInfo->domainInfo ==
                UMTS_LAYER3_CN_DOMAIN_PS)
            {
                //UmtsSetBit(*(ueL3->nsapiBitSet), appInfo->nsapi);
                ueL3->nsapiBitSet->set(appInfo->nsapi);
            }
            newApp = TRUE;
        }
    }

    appInfo->lastActiveTime = node->getNodeTime();

    // get/update QoS  info
    appType = (TraceProtocolType) msg->originatingProtocol;

    UmtsLayer3GetFlowQoSInfo(
        node, interfaceIndex, appType,
        tos, &payload, &(appInfo->qosInfo));

    // update stat
    if (newApp)
    {
        if (appInfo->classifierInfo->domainInfo ==
            UMTS_LAYER3_CN_DOMAIN_PS)
        {
            ueL3->stat.numMoAppRcvdFromUpper ++;

            // PS domain
            if (appInfo->qosInfo->trafficClass ==
                UMTS_QOS_CLASS_CONVERSATIONAL)
            {
                ueL3->stat.numMoConversationalAppRcvdFromUpper ++;
            }
            else if (appInfo->qosInfo->trafficClass ==
                UMTS_QOS_CLASS_STREAMING)
            {
                ueL3->stat.numMoStreamingAppRcvdFromUpper ++;
            }
            else if (appInfo->qosInfo->trafficClass ==
                UMTS_QOS_CLASS_BACKGROUND)
            {
                ueL3->stat.numMoBackgroundAppRcvdFromUpper ++;
            }
            else if (appInfo->qosInfo->trafficClass >
                UMTS_QOS_CLASS_STREAMING &&
                appInfo->qosInfo->trafficClass <
                UMTS_QOS_CLASS_BACKGROUND)
            {
                ueL3->stat.numMoInteractiveAppRcvdFromUpper ++;
            }
        }
    }

    if (DEBUG_QOS && newApp)
    {
        char clkStr[MAX_STRING_LENGTH];

        if (appInfo->classifierInfo->domainInfo ==
            UMTS_LAYER3_CN_DOMAIN_CS)
        {
            UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_CC);
        }
        else if (appInfo->classifierInfo->domainInfo ==
                 UMTS_LAYER3_CN_DOMAIN_PS)
        {
            UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_SM);

        }
        TIME_PrintClockInSecond(
            appInfo->qosInfo->transferDelay, clkStr);

        printf("\n\t handle packet from upper for a new flow\n");
        printf("\n\t -------QoS Info------");
        printf("\n\t trafficClass %d \t UL maxBitRate %d"
             "\t UL guaranteedBitRate %d",
             appInfo->qosInfo->trafficClass,
             appInfo->qosInfo->ulMaxBitRate,
             appInfo->qosInfo->ulGuaranteedBitRate);
        printf("\n\t DL maxBitRate %d DL guaranteedBitRate %d \n"
             "\t sduErrorRatio %f \t residualBER %f",
             appInfo->qosInfo->dlMaxBitRate,
             appInfo->qosInfo->dlGuaranteedBitRate,
             (double)(appInfo->qosInfo->sduErrorRatio),
             appInfo->qosInfo->residualBER);
       printf(" \t transferDelay %s\n",
             clkStr);
    }

    if (appInfo->classifierInfo->domainInfo == UMTS_LAYER3_CN_DOMAIN_CS)
    {
        UmtsLayer3UeHandleCsPacketFromUpper(node, umtsL3, ueL3, appInfo, msg);
    }
    else if (appInfo->classifierInfo->domainInfo == UMTS_LAYER3_CN_DOMAIN_PS)
    {
        UmtsLayer3UeHandlePsPacketFromUpper(node, umtsL3, ueL3, appInfo, msg);
    }
    else
    {
        ERROR_ReportWarning("rcvd a unknow domain packet from upper");
        MESSAGE_Free(node, msg);

    }
}
