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

#include "api.h"
#include "partition.h"
#include "network.h"
#include "network_ip.h"
#include "ipv6.h"

#include "cellular.h"
#include "cellular_layer3.h"
#include "cellular_umts.h"
#include "layer3_umts.h"
#include "layer3_umts_gsn.h"
#include "layer3_umts_hlr.h"

#define DEBUG_UE_ID        0 // default be -1
#define DEBUG_PARAMETER    0
#define DEBUG_BACKBONE     0
#define DEBUG_MM           0
#define DEBUG_GMM          0
#define DEBUG_CC           0
#define DEBUG_SM           0
#define DEBUG_RANAP        0
#define DEBUG_PACKET       0
#define DEBUG_GTP          0
#define DEBUG_IUR          0
#define DEBUG_QOS          0
#define DEBUG_ASSERT       0  // some ERROR_Assert will be triggered
#define DEBUG_CC_DATA      0
#define DEBUG_GTP_DATA     0

// /**
// FUNCTION   :: UmtsLayer3GsnLookupVlr
// LAYER      :: Layer 3
// PURPOSE    :: Lookup VLR information for a given UE indicated by UE ID
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + ueId      : UInt32           : Identification of the UE
// RETURN     :: UmtsVlrEntry *   : Pointer to the VLR entry if found in VLR
//                                  NULL otherwise
// **/
static
UmtsVlrEntry *UmtsLayer3GsnLookupVlr(Node *node,
                                     UmtsLayer3Data *umtsL3,
                                     UInt32 ueId);

// /**
// FUNCTION   :: UmtsLayer3GsnLookupVlr
// LAYER      :: Layer 3
// PURPOSE    :: Lookup VLR information for a given UE indicated by UE ID
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + ueId      : UInt32           : Identification of the UE
// + rncId     : UInt32 *         : For return ID of the RNC controls UE
// + cellId    : UInt32 *         : For return cell ID where UE is located
// RETURN     :: BOOL : TRUE if found in VLR, FALSE otherwise
// **/
static
BOOL UmtsLayer3GsnLookupVlr(Node *node,
                            UmtsLayer3Data *umtsL3,
                            UInt32 ueId,
                            UInt32 *rncId,
                            UInt32 *cellId);

// /**
// FUNCTION   :: UmtsLayer3GgsnCheckPdpPendingOnRouting
// LAYER      :: Layer 3
// PURPOSE    :: This function is called when getting routing info from HLR
// PARAMETERS ::
// + node      : Node*               : Pointer to node.
// + umtsL3    : UmtsLayer3Data *    : Pointer to UMTS Layer3 data
// + ueImsi    : CellularIMSI        : IMSI of the UE
// + locInfo   : UmtsHlrLocationInfo*: Location info of the UE from HLR.
//                                     NULL means HLR lookup failed
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3GgsnCheckPdpPendingOnRouting(Node *node,
                                            UmtsLayer3Data *umtsL3,
                                            CellularIMSI ueImsi,
                                            UmtsHlrLocationInfo *locInfo);
// /**
// FUNCTION   :: UmtsLayer3GmscCheckCallPendingOnRouting
// LAYER      :: Layer 3
// PURPOSE    :: This function is called when getting routing info from HLR
// PARAMETERS ::
// + node      : Node*               : Pointer to node.
// + umtsL3    : UmtsLayer3Data *    : Pointer to UMTS Layer3 data
// + ueId      : NodeId              : UE ID
// + locInfo   : UmtsHlrLocationInfo*: Location info of the UE from HLR.
//                                     NULL means HLR lookup failed
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3GmscCheckCallPendingOnRouting(Node *node,
                                             UmtsLayer3Data *umtsL3,
                                             NodeId ueId,
                                             UmtsHlrLocationInfo *locInfo);

// /**
// FUNCTION   :: UmtsLayer3GsnSmSendRequestPDPContextActivation
// LAYER      :: Layer3 GMM
// PURPOSE    :: Send a Attach request
// PARAMETERS ::
// + node      : Node*             : Pointer to node.
// + umtsL3    : UmtsLayer3Data*   : Pointer to L3 data
// + gsnL3     : UmtsLayer3Gsn *   : gsn data
// + ueId      : UInt32 : The UE which are ready for sending the req
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3GsnSmSendRequestPDPContextActivation(
         Node* node,
         UmtsLayer3Data* umtsL3,
         UmtsLayer3Gsn *gsnL3,
         NodeId ueId);

// FUNCTION   :: UmtsLayer3GsnMmConnEstInd
// LAYER      :: Layer3 MM
// PURPOSE    :: Request to establish MM connection by CM entities
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data*   : Pointer to L3 data
// + ueNas            : UmtsSgsnUeDataInSgsn* : Pointer to NAS
// RETURN     :: void
// **/
static
void UmtsLayer3GsnMmConnEstInd(
        Node* node,
        UmtsLayer3Data *umtsL3,
        UmtsSgsnUeDataInSgsn* ueNas);

// FUNCTION   :: UmtsLayer3GsnMmConnRelInd
// LAYER      :: Layer3 MM
// PURPOSE    :: Request to establish MM connection by CM entities
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data*   : Pointer to L3 data
// + ueNas            : UmtsSgsnUeDataInSgsn* : Pointer to NAS
// + pd               : UmtsLayer3PD      : Protocol discrimator
// + ti               : unsigned char     : Transaction ID
// RETURN     :: BOOL : TRUE if the MM connection is active
// **/
static
void UmtsLayer3GsnMmConnRelInd(
        Node* node,
        UmtsLayer3Data *umtsL3,
        UmtsSgsnUeDataInSgsn* ueNas,
        UmtsLayer3PD pd,
        unsigned char ti);

// /**
// FUNCTION   :: UmtsLayer3GsnGtpSendPduNotificationResponse
// LAYER      :: Layer 3
// PURPOSE    :: Send a PDU Notification Response to GGSN
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + gsnL3     : UmtsLayer3Gsn*         : GSN data
// + ueInfo    : UmtsSgsnUeDataInSgsn*  : Info of UE at the SGSN
// + flowInfo  : UmtsLayer3SgsnFlowInfo*: Info of the flow at SGSN
// + gtpCause  : UmtsGtpCauseType       : Result of the request
// + gsnAddr   : Address                : Address of SGSN/GGSN
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3GsnGtpSendPduNotificationResponse(
         Node*node,
         UmtsLayer3Data* umtsL3,
         UmtsLayer3Gsn* gsnL3,
         UmtsSgsnUeDataInSgsn* ueInfo,
         UmtsLayer3SgsnFlowInfo* flowInfo,
         UmtsGtpCauseType gtpCause,
         Address gsnAddr);

#if 0
static
void UmtsLayer3GgsnRemovePdpInfoByTeId(
                           Node *node,
                           UmtsLayer3Data *umtsL3,
                           UmtsLayer3Gsn* gsnL3,
                           UInt32 teId,
                           NodeId* ueId,
                           BOOL upLink);
#endif
//
//*** Utility Functions
//
// /**
// FUNCTION   :: UmtsLayer3GmscIsCallAddrInMyPlmn
// LAYER      :: Layer3
// PURPOSE    :: Check if a voice address in the PLMN controled by the GMSC
// PARAMETERS ::
// + node      : Node*          : Pointer to node.
// + gsnL3     : UmtsLayer3Gsn* : Pointer to the GSN data
// + callAddr  : NodeId         : The call address to be checked
// RETURN     :: BOOL : TRUE if the call address in my PLMN, FALSE otherwise
// **/
BOOL UmtsLayer3GmscIsCallAddrInMyPlmn(
        Node* node,
        UmtsLayer3Gsn* gsnL3,
        NodeId callAddr)
{
    //TODO: Check if the node is an PTSN/Cellur node,
    //
    //TODO: check if the node is within in the current PLMN according to the
    //number, may need to change the type of
    // callAddr from NodeId to a Number type
    return TRUE;
}

// /**
// FUNCTION   :: UmtsLayer3GsnInitUeMm
// LAYER      :: Layer 3
// PURPOSE    :: Initialize NW MM data
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data*  : Pint to the umts L3 data
// + gsnL3     : UmtsLayer3Gsn *gsnL3 : Pointer to UMTS gdn Layer3 data
// RETURN     :: UmtsMmNwData*    :
// **/
UmtsMmNwData* UmtsLayer3GsnInitUeMm(
                Node* node,
                UmtsLayer3Data* umtsl3,
                UmtsLayer3Gsn* gsnL3)
{
    UmtsMmNwData* ueMm = (UmtsMmNwData*)(MEM_malloc(sizeof(UmtsMmNwData)));
    memset(ueMm, 0, sizeof(UmtsMmNwData));

    ueMm->mainState = UMTS_MM_NW_IDLE;
    ueMm->actMmConns = new UmtsMmConnCont;
    //ueMm->rqtingMmConns = new rqtingMmConns;
    return ueMm;
}

// /**
// FUNCTION   :: UmtsLayer3GsnFinalizeUeMm
// LAYER      :: Layer 3
// PURPOSE    :: Initialize NW MM data
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data*  : Pint to the umts L3 data
// + ueMm      : UmtsMmNwData*    : UE MM data
// RETURN     :: void    : NULL
// **/
void UmtsLayer3GsnFinalizeUeMm(
      Node* node,
      UmtsLayer3Data* umtsl3,
      UmtsMmNwData* ueMm)
{
    UmtsClearStlContDeep(ueMm->actMmConns);
    delete ueMm->actMmConns;
    //UmtsClearStlContDeep(ueMm->rqtingMmConns);
    //delete ueMm->rqtingMmConns;

    if (ueMm->timerT3255)
    {
        MESSAGE_CancelSelfMsg(node, ueMm->timerT3255);
        ueMm->timerT3255 = NULL;
    }
    MEM_free(ueMm);
}

// /**
// FUNCTION   :: UmtsLayer3MscStopCcTimers
// LAYER      :: Layer 3
// PURPOSE    :: Stop active CC timers
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + ueCc      : UmtsCcMsData*    : Pointer to the CC entity data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3MscStopCcTimers(Node* node,
                               UmtsCcNwData* ueCc)
{
    if (ueCc->T301)
    {
        MESSAGE_CancelSelfMsg(node, ueCc->T301);
        ueCc->T301 = NULL;
    }
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
// FUNCTION   :: UmtsLayer3GsnInitUeCc
// LAYER      :: Layer 3
// PURPOSE    :: Initialize NW CC data
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + gsnL3     : UmtsLayer3Gsn    : Pointer to UMTS gdn Layer3 data
// + ueFlow    : UmtsLayer3SgsnFlowInfo* : Pointer to the app flow info
// RETURN     :: void    : NULL
// **/
void UmtsLayer3GsnInitUeCc(
        Node* node,
        UmtsLayer3Gsn* gsnL3,
        UmtsLayer3SgsnFlowInfo* ueFlow)
{
    ueFlow->cmData.ccData =
        (UmtsCcNwData*)MEM_malloc(sizeof(UmtsCcNwData));
    memset((void*)(ueFlow->cmData.ccData), 0, sizeof(UmtsCcNwData));
    ueFlow->cmData.ccData->state = UMTS_CC_NW_NULL;
}

// /**
// FUNCTION   :: UmtsLayer3GsnFinalizeUeCc
// LAYER      :: Layer 3
// PURPOSE    :: Finalize NW CC data
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + gsnL3     : UmtsLayer3Gsn    : Pointer to UMTS gdn Layer3 data
// + ueFlow    : UmtsLayer3SgsnFlowInfo* : Pointer to the app flow info
// RETURN     :: void    : NULL
// **/
void UmtsLayer3GsnFinalizeUeCc(
        Node* node,
        UmtsLayer3Gsn* gsnL3,
        UmtsLayer3SgsnFlowInfo* ueFlow)
{
    UmtsCcNwData* ueCc = ueFlow->cmData.ccData;
    UmtsLayer3MscStopCcTimers(node, ueCc);
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
    ueFlow->cmData.ccData = NULL;
}

// /**
// FUNCTION   :: UmtsLayer3GsnAppInfoFinalize
// LAYER      :: Layer 3
// PURPOSE    :: UmtsLayer3SgsnFlowInfo destructor
// + node      : Node*             : Pointer to node.
// + gsnL3     : UmtsLayer3Gsn*    : Pointer to the umts gsn L3 data
// + appInfo   : UmtsLayer3SgsnFlowInfo* : Pointer to the flow to be removed
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3GsnAppInfoFinalize(
           Node* node,
           UmtsLayer3Gsn* gsnL3,
           UmtsLayer3SgsnFlowInfo* appInfo)
{
    if (appInfo->classifierInfo)
    {
        MEM_free(appInfo->classifierInfo);
    }
    if (appInfo->cmData.ccData)
    {
        UmtsLayer3GsnFinalizeUeCc(node, gsnL3, appInfo);
    }
    if (appInfo->cmData.smData)
    {
        if (appInfo->cmData.smData->T3385)
        {
            MESSAGE_CancelSelfMsg(node, appInfo->cmData.smData->T3385);
            appInfo->cmData.smData->T3385 = NULL;
        }
        if (appInfo->cmData.smData->T3395)
        {
            MESSAGE_CancelSelfMsg(node, appInfo->cmData.smData->T3395);
            appInfo->cmData.smData->T3395 = NULL;
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

    // free packet in buffer
    while (!appInfo->inPacketBuf->empty())
    {
        Message* toFree = appInfo->inPacketBuf->front();
        appInfo->inPacketBuf->pop();
        MESSAGE_Free(node, toFree);
    }
    delete appInfo;
}

// /**
// FUNCTION   :: UmtsLayer3GgsnPdpInfoFinalize
// LAYER      :: Layer 3
// PURPOSE    :: Handle a GTP msg at SGSN
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + gsnL3     : UmtsLayer3Gsn*    : Pointer to the umts gsn L3 data
// + pdpInfo   : UmtsLayer3GgsnPdpInfo * : poninter to the PDP info
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3GgsnPdpInfoFinalize(
                           Node *node,
                           UmtsLayer3Gsn *gsnL3,
                           UmtsLayer3GgsnPdpInfo* pdpInfo)
{
    // free classifier
    if (pdpInfo->classifierInfo)
    {
        MEM_free(pdpInfo->classifierInfo);
    }
    // free QoS
    if (pdpInfo->qosInfo)
    {
        MEM_free(pdpInfo->qosInfo);
    }
    // TODO: free apcket in the buffer
    // free packet in buffer
    while (!pdpInfo->inPacketBuf->empty())
    {
        Message* toFree = pdpInfo->inPacketBuf->front();
        pdpInfo->inPacketBuf->pop();
        gsnL3->agregPktBufSize -= MESSAGE_ReturnPacketSize(toFree);
        MESSAGE_Free(node, toFree);

        // update statistics
        gsnL3->ggsnStat.numPsDataPacketDropped ++;
    }
    pdpInfo->pktBufSize = 0;

    delete pdpInfo;
}

// /**
// FUNCTION   :: UmtsLayer3GgsnEnquePsPktFromOutside
// LAYER      :: Layer 3
// PURPOSE    :: Enqueue pckets from upper
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + gsnL3     : UmtsLayer3Gsn*   : Pointer to the umts gsn L3 data
// + appInfo   : UmtsLayer3GgsnPdpInfo * : poninter to the PDP info
// + msg       : Message*         : message to be handled
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3GgsnEnquePsPktFromOutside(
        Node* node,
        UmtsLayer3Gsn *gsnL3,
        UmtsLayer3GgsnPdpInfo* appInfo,
        Message* msg)
{
    if (appInfo->pktBufSize + MESSAGE_ReturnPacketSize(msg)
            < UMTS_GGSN_MAXBUFSIZE_PERFLOW
            && gsnL3->agregPktBufSize + MESSAGE_ReturnPacketSize(msg)
                < UMTS_GGSN_MAXBUFSIZE)
    {
        appInfo->inPacketBuf->push(msg);
        appInfo->pktBufSize += MESSAGE_ReturnPacketSize(msg);
        gsnL3->agregPktBufSize += MESSAGE_ReturnPacketSize(msg);
    }
    else
    {
        if (appInfo->qosInfo->trafficClass >= UMTS_QOS_CLASS_STREAMING)
        {
            MESSAGE_Free(node, msg);
        }
        else
        {
            appInfo->inPacketBuf->push(msg);
            appInfo->pktBufSize += MESSAGE_ReturnPacketSize(msg);
            gsnL3->agregPktBufSize += MESSAGE_ReturnPacketSize(msg);
            while (appInfo->pktBufSize > UMTS_GGSN_MAXBUFSIZE_PERFLOW ||
                    gsnL3->agregPktBufSize > UMTS_GGSN_MAXBUFSIZE)
            {
                Message* toFree = appInfo->inPacketBuf->front();
                appInfo->pktBufSize -= MESSAGE_ReturnPacketSize(toFree);
                gsnL3->agregPktBufSize -= MESSAGE_ReturnPacketSize(toFree);
                MESSAGE_Free(node, toFree);
                appInfo->inPacketBuf->pop();
            }
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3GsnAddUeInfo
// LAYER      :: Layer 3
// PURPOSE    :: Initialize NW GMM data
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data*  : Pint to the umts L3 data
// + gsnL3     : UmtsLayer3Gsn *gsnL3 : Pointer to UMTS gdn Layer3 data
// + ueId      : NodeId                 : UE ID
// + rncId     : NodeId                 : RNC ID
// + imsi      : CellularIMSI           : IMSI of the UE
// RETURN     :: UmtsSgsnUeDataInSgsn* : NULL
// **/
static
UmtsSgsnUeDataInSgsn* UmtsLayer3GsnAddUeInfo(Node *node,
                            UmtsLayer3Data* umtsL3,
                            UmtsLayer3Gsn *gsnL3,
                            NodeId ueId,
                            NodeId rncId,
                            CellularIMSI imsi)
{
    UmtsSgsnUeDataInSgsn* ueInfo =
            (UmtsSgsnUeDataInSgsn *)
            MEM_malloc(sizeof(UmtsSgsnUeDataInSgsn));
    memset((void*)ueInfo, 0, sizeof(UmtsSgsnUeDataInSgsn));

    ueInfo->appInfoList = new std::list<UmtsLayer3SgsnFlowInfo*>;

    // allocate a TMSI/P-TMSI
    ueInfo->ueId = ueId;
    ueInfo->imsi = imsi;
    ueInfo->tmsi = ueId; // TODO
    ueInfo->rncId = rncId;
    ueInfo->gmmData = (UmtsGMmNwData*)(MEM_malloc(sizeof(UmtsGMmNwData)));
    memset(ueInfo->gmmData, 0, sizeof(UmtsGMmNwData));
    ueInfo->gmmData->mainState = UMTS_GMM_NW_DEREGISTERED;
    ueInfo->gmmData->pmmState = UMTS_PMM_DETACHED;

    ueInfo->mmData = UmtsLayer3GsnInitUeMm(node, umtsL3, gsnL3);
    ueInfo->transIdBitSet = new std::bitset<UMTS_MAX_TRANS_MOBILE>;

    gsnL3->ueInfoList->push_back(ueInfo);

    return ueInfo;
}

// /**
// FUNCTION   :: UmtsLayer3GsnRemoveUeInfo
// LAYER      :: Layer 3
// PURPOSE    :: Initialize NW GMM data
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data*  : Pint to the umts L3 data
// + gsnL3     : UmtsLayer3Gsn *gsnL3 : Pointer to UMTS gdn Layer3 data
// + ueId      : NodeId                 : UE ID
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3GsnRemoveUeInfo(Node *node,
                               UmtsLayer3Data* umtsL3,
                               UmtsLayer3Gsn *gsnL3,
                               NodeId ueId)
{
    // to see if the IMSI has been seen
    std::list<UmtsSgsnUeDataInSgsn*>::iterator it;
    for (it = gsnL3->ueInfoList->begin();
        it != gsnL3->ueInfoList->end();
        it ++)
    {
        if ((*it)->tmsi == ueId)
        {
            break;
        }
    }
    if (it != gsnL3->ueInfoList->end())
    {
        std::list<UmtsLayer3SgsnFlowInfo*>::iterator appIter;
        for (appIter = (*it)->appInfoList->begin();
             appIter != (*it)->appInfoList->end();
             ++appIter)
        {
            UmtsLayer3GsnAppInfoFinalize(node, gsnL3, (*appIter));
        }
        delete (*it)->appInfoList;
        delete (*it)->transIdBitSet;

        if ((*it)->gmmData->T3313)
        {
            MESSAGE_CancelSelfMsg(node, (*it)->gmmData->T3313);
            (*it)->gmmData->T3313 = NULL;
        }
        if ((*it)->gmmData->T3350)
        {
            MESSAGE_CancelSelfMsg(node, (*it)->gmmData->T3350);
            (*it)->gmmData->T3350 = NULL;
        }
        MEM_free((*it)->gmmData);
        (*it)->gmmData = NULL;

        UmtsLayer3GsnFinalizeUeMm(node, umtsL3, (*it)->mmData);

        // free the mmory
        MEM_free(*it);

        gsnL3->ueInfoList->erase(it);
    }
}

// /**
// FUNCTION   :: UmtsLayer3GmscFindCallFlow
// LAYER      :: Layer 3
// PURPOSE    :: Search for a call flow in GMSC by UE ID and TRANSACTION ID
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + gsnL3     : UmtsLayer3Gsn*   : GSN layer 3 data
// + ueId      : NodeId           : UE ID
// + ti        : char             : transaction ID
// RETURN     :: UmtsLayer3GmscCallInfo : The call flow to be retrieved
static
UmtsLayer3GmscCallInfo* UmtsLayer3GmscFindCallFlow(
                           Node *node,
                           UmtsLayer3Gsn* gsnL3,
                           NodeId ueId,
                           char ti)
{
    UmtsLayer3GmscCallInfo* callFlow = NULL;
    if (gsnL3->isGGSN)
    {
        UmtsGmscCallFlowList::iterator callFlowIter
                        = gsnL3->callFlowList->begin();
        for (callFlowIter = gsnL3->callFlowList->begin();
             callFlowIter != gsnL3->callFlowList->end();
             ++ callFlowIter)
        {
            if ((*callFlowIter)->ueId == ueId
                && (*callFlowIter)->ti == ti)
            {
                callFlow = *callFlowIter;
                break;
            }
        }
    }
    return callFlow;
}

// /**
// FUNCTION   :: UmtsLayer3GmscFindCallFlow
// LAYER      :: Layer 3
// PURPOSE    :: Search for a UE call flow in GMSC by UE ID and peer address
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + gsnL3     : UmtsLayer3Gsn*   : GSN layer 3 data
// + ueId      : NodeId           : UE ID
// + peerAddr  : NodeId           : Peer Address
// RETURN     :: UmtsLayer3GmscCallInfo : The call flow to be retrieved
static
UmtsLayer3GmscCallInfo* UmtsLayer3GmscFindCallFlow(
                           Node *node,
                           UmtsLayer3Gsn* gsnL3,
                           NodeId ueId,
                           NodeId peerAddr)
{
    UmtsLayer3GmscCallInfo* callFlow = NULL;
    if (gsnL3->isGGSN)
    {
        UmtsGmscCallFlowList::iterator callFlowIter
                        = gsnL3->callFlowList->begin();
        for (callFlowIter = gsnL3->callFlowList->begin();
             callFlowIter != gsnL3->callFlowList->end();
             ++ callFlowIter)
        {
            if ((*callFlowIter)->ueId == ueId
                && (*callFlowIter)->peerAddr == peerAddr)
            {
                callFlow = *callFlowIter;
                break;
            }
        }
    }
    return callFlow;
}

// /**
// FUNCTION   :: UmtsLayer3GmscAddCallFlow
// LAYER      :: Layer 3
// PURPOSE    :: Add a new call flow into the list kept at GMSC
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + gsnL3     : UmtsLayer3Gsn*   : GSN layer 3 data
// + ueId      : NodeId           : UE ID
// + ti        : char             : transaction ID
// + peerAddr  : NodeId           : Peer Address
// + sgsnId    : NodeId           : The MSC(sgsn) which the UE
//                                  is connected with
// RETURN     :: UmtsLayer3GmscCallInfo : The call flow to be retrieved
static
UmtsLayer3GmscCallInfo* UmtsLayer3GmscAddCallFlow(
                               Node *node,
                               UmtsLayer3Gsn* gsnL3,
                               NodeId ueId,
                               NodeId peerAddr,
                               int appId,
                               clocktype endTime,
                               clocktype avgTalkTime,
                               char ti = -1,
                               const Address* mscAddr = NULL)
{
    UmtsLayer3GmscCallInfo* callFlow = new UmtsLayer3GmscCallInfo(
                                                ueId,
                                                ti,
                                                peerAddr,
                                                appId,
                                                endTime,
                                                avgTalkTime,
                                                mscAddr);

    gsnL3->callFlowList->push_back(callFlow);
    return callFlow;
}

// FUNCTION   :: UmtsLayer3SgsnSendRanapMsgToRnc
// LAYER      :: Layer 3
// PURPOSE    :: Send a RANAP message to one RNC
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msg       : Message*               : Message containing the NBAP msg
// + ueId      : NodeId                 : UE ID
// + msgType   : UmtsRanapMessageType   : RANAP message type
// + rncId     : NodeId      : The RNC ID the message will be sent to
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3SgsnSendRanapMsgToRnc(
    Node *node,
    UmtsLayer3Data *umtsL3,
    Message *msg,
    NodeId nodeId,
    UmtsRanapMessageType msgType,
    NodeId rncId)
{
    UmtsLayer3Gsn *gsnL3 = (UmtsLayer3Gsn *)(umtsL3->dataPtr);

    UmtsAddIuBackboneHeader(node,
                            msg,
                            nodeId,
                            msgType);

    UmtsLayer3RncInfo* rncInfo = NULL;
    rncInfo = (UmtsLayer3RncInfo *)
                    FindItem(node,
                             gsnL3->rncList,
                             0,
                             (char *) &(rncId),
                             sizeof(rncId));
    ERROR_Assert(rncInfo, "UmtsLayer3SgsnSendRanapMsgToRnc: "
        "cannot find RNC info. ");

    if (rncInfo->nodeAddr.networkType == NETWORK_IPV4)
    {
        NetworkIpSendRawMessageToMacLayerWithDelay(
            node,
            msg,
            NetworkIpGetInterfaceAddress(
                    node, rncInfo->interfaceIndex),
            GetIPv4Address(rncInfo->nodeAddr),
            IPTOS_PREC_INTERNETCONTROL,
            IPPROTO_CELLULAR,
            1,
            rncInfo->interfaceIndex,
            GetIPv4Address(rncInfo->nodeAddr),
            10 * MILLI_SECOND);
    }
    else if (rncInfo->nodeAddr.networkType == NETWORK_IPV6)
    {
        in6_addr srcAddr;
        Ipv6GetGlobalAggrAddress(node,
                                 rncInfo->interfaceIndex,
                                 &srcAddr);
        Ipv6SendRawMessageToMac(
            node,
            msg,
            srcAddr,
            GetIPv6Address(rncInfo->nodeAddr),
            rncInfo->interfaceIndex,
            IPTOS_PREC_INTERNETCONTROL,
            IPPROTO_CELLULAR,
            1);
    }
    else
    {
        ERROR_ReportError("UMTS: Unknow destnation address type!");
    }

#if 0
    UmtsLayer3SendMsgOverBackbone(
        node,
        msg,
        rncInfo->nodeAddr,
        rncInfo->interfaceIndex,
        IPTOS_PREC_INTERNETCONTROL,
        1);
#endif

}

// /**
// FUNCTION   :: UmtsLayer3SgsnSendIurForwardMsgToSgsn
// LAYER      :: Layer 3
// PURPOSE    :: forward a Iur msg to a sgsn where the target RNC locates
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msg       : Message*        : Message containing the NBAP message
// + ueId      : NodeId                 : UE ID
// + msgType   : UmtsRanapMessageType   : RANAP message type
// + rncId     : NodeId          : The RNC ID the message will be sent to
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3SgsnSendIurForwardMsgToSgsn(
    Node *node,
    UmtsLayer3Data *umtsL3,
    Message *msg,
    NodeId destRncId,
    const Address& sgsnAddr)
{
    UmtsAddBackboneHeader(node,
                          msg,
                          UMTS_BACKBONE_MSG_TYPE_IUR_FORWARD,
                          (char*)&destRncId,
                          sizeof(destRncId));

    UmtsLayer3SendMsgOverBackbone(
        node,
        msg,
        sgsnAddr,
        ANY_INTERFACE,
        IPTOS_PREC_INTERNETCONTROL,
        IPDEFTTL);
}

// /**
// FUNCTION   :: UmtsLayer3SgsnSendMsgToHlr
// LAYER      :: Layer 3
// PURPOSE    :: Send a RANAP message to the HLR node
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + msg       : Message*         : Message containing the NBAP message
// + hlrAddr   : Address          : Address of the HLR
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3SgsnSendMsgToHlr(Node *node,
                                UmtsLayer3Data *umtsL3,
                                Message *msg,
                                Address hlrAddr,
                                UmtsHlrCmdTgt cmdTgt = UMTS_HLR_COMMAND_MS)
{
    //UmtsLayer3Gsn *gsnL3 = (UmtsLayer3Gsn *)(umtsL3->dataPtr);

    char info = (char)cmdTgt;
    UmtsAddBackboneHeader(node,
                          msg,
                          UMTS_BACKBONE_MSG_TYPE_GR,
                          &info,
                          sizeof(info));

    UmtsLayer3SendMsgOverBackbone(
        node,
        msg,
        hlrAddr,
        ANY_INTERFACE,
        IPTOS_PREC_INTERNETCONTROL,
        IPDEFTTL);
}

// /**
// FUNCTION   :: UmtsLayer3SgsnSendHelloMsgToGgsn
// LAYER      :: Layer 3
// PURPOSE    :: Send a hello message to the GGSN node
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + ggsnAddr  : Address          : Address of the GGSN node
// RETURN     :: void             : NULL
// **/
static
void UmtsLayer3SgsnSendHelloMsgToGgsn(Node *node,
                                      UmtsLayer3Data *umtsL3,
                                      Address ggsnAddr)
{
    UmtsLayer3Gsn *gsnL3 = (UmtsLayer3Gsn *)(umtsL3->dataPtr);
    UmtsLayer3AddressInfo *addrInfo;
    ListItem *item;
    std::string buffer;
    //int interfaceIndex;
    Message *msg;
    UmtsLayer3HelloPacket *helloPkt;

    // this hello is maintain to tell GGSN the address scope
    // allocated to UEs under control of this SGSN. In the real systesms
    // it is GGSN who allocates such addresses from its pool. In QualNet,
    // addresses are allocated statically at the beginning.

    item = gsnL3->addrInfoList->first;
    while (item)
    {
        addrInfo = (UmtsLayer3AddressInfo *)item->data;
        buffer.append((char*)&addrInfo->subnetAddress,
                      sizeof(addrInfo->subnetAddress));
        buffer.append((char*)&addrInfo->numHostBits,
                      sizeof(addrInfo->numHostBits));
        item = item->next;
    }

    // allocate the message and send out
    msg = MESSAGE_Alloc(node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        msg,
                        (int)(sizeof(UmtsLayer3HelloPacket) + buffer.size()),
                        TRACE_UMTS_LAYER3);

    helloPkt = (UmtsLayer3HelloPacket *) MESSAGE_ReturnPacket(msg);
    ERROR_Assert(helloPkt != NULL, "UMTS: Memory error!");

    helloPkt->nodeType = umtsL3->nodeType;
    helloPkt->nodeId = node->nodeId;

    memcpy(MESSAGE_ReturnPacket(msg) + sizeof(UmtsLayer3HelloPacket),
           buffer.data(),
           buffer.size());

    UmtsLayer3AddHeader(node,
                        msg,
                        UMTS_LAYER3_PD_SIM,
                        0,
                        0);

    // Add a backbone header
    UmtsAddBackboneHeader(node,
                          msg,
                          UMTS_BACKBONE_MSG_TYPE_HELLO,
                          NULL,
                          0);

    // send out the packet
    UmtsLayer3SendMsgOverBackbone(
        node,
        msg,
        ggsnAddr,
        ANY_INTERFACE,
        IPTOS_PREC_INTERNETCONTROL,
        IPDEFTTL);
}

// /**
// FUNCTION   :: UmtsLayer3GsnSendNasMsg
// LAYER      :: Layer 3
// PURPOSE    :: Send a NAS stratum message a UE
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + nasMsg    : char*                  : NAS message content
// + nasMsgSize: int                    : NAS message size
// + pd        : char                   : Protocol discriminator
// + msgType   : char                   : NAS message type
// + transctId : char                   : Transaction ID
// + tmsi      : NodeId                 : UE ID
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3GsnSendNasMsg(
    Node*           node,
    UmtsLayer3Data* umtsL3,
    char*           nasMsg,
    int             nasMsgSize,
    char            pd,
    char            msgType,
    unsigned int    transactId,
    NodeId          tmsi)
{
    UmtsLayer3Header l3Header;
    l3Header.tiSpd = transactId;
    l3Header.pd = pd;
    l3Header.msgType = msgType;

    UmtsLayer3CnDomainId domain;
    if (pd == (char)UMTS_LAYER3_PD_CC ||
            pd == (char)UMTS_LAYER3_PD_MM)
    {
        domain = UMTS_LAYER3_CN_DOMAIN_CS;
    }
    else
    {
        domain = UMTS_LAYER3_CN_DOMAIN_PS;
    }
    int packetSize = nasMsgSize + sizeof(UmtsLayer3Header) + 1;
    Message* msg = MESSAGE_Alloc(
        node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        msg,
                        packetSize,
                        TRACE_UMTS_LAYER3);
    char* packet = MESSAGE_ReturnPacket(msg);
    (*packet) = (char)domain;
    memcpy(packet + 1, &l3Header, sizeof(UmtsLayer3Header));
    memcpy(packet + 1 + sizeof(UmtsLayer3Header), nasMsg, nasMsgSize);

    //TODO: Find the RNC information that the UE is connected into
    NodeId rncId;
    NodeId ueId = tmsi; // TODO: conversion needed
    UInt32 cellId;

    UmtsLayer3GsnLookupVlr(node,
                           umtsL3,
                           ueId,
                           &rncId,
                           &cellId);

    UmtsLayer3SgsnSendRanapMsgToRnc(
        node,
        umtsL3,
        msg,
        ueId,
        UMTS_RANAP_MSG_TYPE_DIRECT_TRANSFER,
        rncId);
}

// /**
// FUNCTION   :: UmtsLayer3GsnPrintParameter
// LAYER      :: Layer 3
// PURPOSE    :: Print out GSN specific parameters
// PARAMETERS ::
// + node      : Node*           : Pointer to node.
// + gsnL3     : UmtsLayer3Gsn * : Pointer to UMTS GSN Layer3 data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3GsnPrintParameter(Node *node, UmtsLayer3Gsn *gsnL3)
{
    char addrStr[MAX_STRING_LENGTH];

    if (gsnL3->isGGSN)
    {
        printf("Node%d(GGSN)'s specific parameters:\n", node->nodeId);
    }
    else
    {
        printf("Node%d(SGSN)'s specific parameters:\n", node->nodeId);
    }

    // print the node ID and address of the HLR
    IO_ConvertIpAddressToString(&(gsnL3->hlrAddr), addrStr);
    printf("    HLR server = %d (%s)\n", gsnL3->hlrNodeId, addrStr);

    // printf SGSN specific parameters
    if (!gsnL3->isGGSN)
    {
        IO_ConvertIpAddressToString(&(gsnL3->ggsnAddr), addrStr);
        printf("    Primary GGSN = %d (%s)\n", gsnL3->ggsnNodeId, addrStr);
    }
    else
    {
        // printf GGSN specific parameters
    }

    fflush(stdout);

    return;
}

// FUNCTION   :: UmtsLayer3GsnSigConnRelInd
// LAYER      :: Layer3 NAS
// PURPOSE    :: Notify MM/GMM one Signal connection is  released
// PARAMETERS ::
// +node      : Node*               : Pointer to node.
// +umtsL3    : UmtsLayer3Data *    : Pointer to UMTS Layer3 data
// RETURN     :: NULL
// **/
static
void UmtsLayer3GsnSigConnRelInd(
    Node* node,
    UmtsLayer3Data *umtsL3,
    UmtsLayer3Gsn* gsnL3,
    NodeId ueId,
    UmtsLayer3CnDomainId domain)
{
    std::list<UmtsSgsnUeDataInSgsn*>::iterator ueNasIter
        = std::find_if(gsnL3->ueInfoList->begin(),
                       gsnL3->ueInfoList->end(),
                       UmtsFindItemByUeId
                            <UmtsSgsnUeDataInSgsn>(ueId));

    if (ueNasIter != gsnL3->ueInfoList->end())
    {
        if (domain == UMTS_LAYER3_CN_DOMAIN_PS)
        {
            (*ueNasIter)->gmmData->pmmState = UMTS_PMM_IDLE;
            //if ((*ueNasIter)->gmmData->T3313)
            //{
            //    MESSAGE_CancelSelfMsg(node, (*ueNasIter)->gmmData->T3313);
            //    (*ueNasIter)->gmmData->T3313 = NULL;
            //}
            if ((*ueNasIter)->gmmData->T3350)
            {
                MESSAGE_CancelSelfMsg(node, (*ueNasIter)->gmmData->T3350);
                (*ueNasIter)->gmmData->T3350 = NULL;
            }

            std::list<UmtsLayer3SgsnFlowInfo*>::iterator it;
            for (it = (*ueNasIter)->appInfoList->begin();
                 it != (*ueNasIter)->appInfoList->end();
                 ++it)
            {
                if ((*it)->classifierInfo->domainInfo == domain)
                {
                    UmtsLayer3SgsnFlowInfo* flowInfo = *it;

                    if (flowInfo->cmData.smData->smState !=
                        UMTS_SM_NW_PDP_REJECTED
                        && flowInfo->cmData.smData->smState !=
                        UMTS_SM_NW_PAGE_PENDING)
                    {
                        UmtsGtpCauseType relCause=
                            UMTS_GTP_REQUEST_REJECTED;

                        if (flowInfo->cmData.smData->smState ==
                            UMTS_SM_NW_PDP_ACTIVE)
                        {
                            relCause = UMTS_GTP_REQUEST_DROPPED;
                        }
                        // Reject the PDP
                        UmtsLayer3GsnGtpSendPduNotificationResponse(
                            node,
                            umtsL3,
                            gsnL3,
                            *ueNasIter,
                            flowInfo,
                            relCause,
                            gsnL3->ggsnAddr);

                        gsnL3->sgsnStat.numPduNotificationRspSent ++;

                        // move state to inactive
                        flowInfo->cmData.smData->smState =
                            UMTS_SM_NW_PDP_REJECTED;
                    }

                    //Copy from RemoveAppInfoList
                    if ((*it)->srcDestType == UMTS_FLOW_NETWORK_ORIGINATED)
                    {
                        (*ueNasIter)->transIdBitSet->reset(
                            (*it)->transId -
                            UMTS_LAYER3_NETWORK_MOBILE_START);
                    }
                    if (flowInfo->cmData.smData->smState !=
                        UMTS_SM_NW_PAGE_PENDING)
                    {
                        UmtsLayer3GsnAppInfoFinalize(node, gsnL3, (*it));
                        *it = NULL;
                    }
                }
            }
            (*ueNasIter)->appInfoList->remove(NULL);
        }
        else if (domain == UMTS_LAYER3_CN_DOMAIN_CS)
        {
            while (!(*ueNasIter)->mmData->actMmConns->empty())
            {
                UmtsLayer3MmConn* mmConn =
                    (*ueNasIter)->mmData->actMmConns->front();
                UmtsLayer3GsnMmConnRelInd(
                    node,
                    umtsL3,
                    (*ueNasIter),
                    mmConn->pd,
                    mmConn->ti);
                delete mmConn;
                (*ueNasIter)->mmData->actMmConns->pop_front();
            }
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3GsnInitParameter
// LAYER      :: Layer 3
// PURPOSE    :: Initialize GSN specific parameters
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + nodeInput : const NodeInput* : Pointer to node input.
// + gsnL3     : UmtsLayer3Gsn *  : Pointer to UMTS GSN Layer3 data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3GsnInitParameter(Node *node,
                                const NodeInput *nodeInput,
                                UmtsLayer3Gsn *gsnL3)
{
    BOOL wasFound;
    BOOL retVal;
    int intVal;
    char errStr[MAX_STRING_LENGTH];
    Address invalidAddr;

    memset(&invalidAddr, 0, sizeof(Address));

    // read the node ID of the HLR
    IO_ReadInt(node->nodeId,
               ANY_ADDRESS,
               nodeInput,
               "UMTS-HLR-SERVER",
               &wasFound,
               &intVal);

    if (wasFound)
    {
        BOOL wrongValue = FALSE;
        char tempStr[MAX_STRING_LENGTH];

        // convert to network address of the HLR
        gsnL3->hlrNodeId = intVal;
        gsnL3->hlrAddr = MAPPING_GetDefaultInterfaceAddressInfoFromNodeId(
                             node,
                             intVal);
        if (memcmp(&(gsnL3->hlrAddr), &invalidAddr, sizeof(Address)) == 0)
        {
            wrongValue = TRUE;
        }

        // check if this node is indeed a HLR node
        IO_ReadString(gsnL3->hlrNodeId,
                      ANY_ADDRESS,
                      nodeInput,
                      "UMTS-NODE-TYPE",
                      &retVal,
                      tempStr);
        if (!retVal || strcmp(tempStr, "HLR") != 0)
        {
            wrongValue = TRUE;
        }

        if (wrongValue)
        {
            sprintf(errStr,
                    "Node%d(GSN): Wrong value of UMTS-HLR-SERVER. It must "
                    "be the node ID of the HLR server.\n",
                    node->nodeId);
            ERROR_ReportError(errStr);
        }
    }
    else
    {
        sprintf(errStr,
           "Node%d(GSN): Parameter UMTS-HLR-SERVER is missing! It must "
                "be configured as the node ID of the HLR server.\n",
                node->nodeId);
        ERROR_ReportError(errStr);
    }

    if (!gsnL3->isGGSN)
    {
        // read parameters specific to SGSN

        // read my primary GGSN node as my gateway to other networks
        IO_ReadInt(node->nodeId,
                   ANY_ADDRESS,
                   nodeInput,
                   "UMTS-PRIMARY-GGSN",
                   &wasFound,
                   &intVal);

        if (wasFound)
        {
            BOOL wrongValue = FALSE;
            char tempStr[MAX_STRING_LENGTH];

            // validate the value and convert to network address
            gsnL3->ggsnNodeId = intVal;
            gsnL3->ggsnAddr =
                MAPPING_GetDefaultInterfaceAddressInfoFromNodeId(
                                  node,
                                  intVal);
            if (!memcmp(&(gsnL3->ggsnAddr), &invalidAddr, sizeof(Address)))
            {
                wrongValue = TRUE;
            }

            // check if this node is indeed a HLR node
            IO_ReadString(gsnL3->ggsnNodeId,
                          ANY_ADDRESS,
                          nodeInput,
                          "UMTS-NODE-TYPE",
                          &retVal,
                          tempStr);
            if (!retVal || strcmp(tempStr, "GGSN") != 0)
            {
                wrongValue = TRUE;
            }

            if (wrongValue)
            {
                sprintf(errStr,
                   "Node%d(SGSN): Wrong value of UMTS-PRIMARY-GGSN. It "
                   "must be the node ID of a GGSN node of the PLMN.\n",
                   node->nodeId);
                ERROR_ReportError(errStr);
            }
        }
        else
        {
            sprintf(errStr,
                "Node%d(SGSN): Parameter UMTS-PRIMARY-GGSN is missing "
                "for this node! It should be configured as the node ID "
                "of a GGSN node of the PLMN.\n",
                node->nodeId);
            ERROR_ReportError(errStr);
        }
    }
    else
    {
        // read some GGSN specific parameters
    }

    return;
}

// /**
// FUNCTION   :: UmtsLayer3SgsnPrintStats
// LAYER      :: Layer 3
// PURPOSE    :: Print out SGSN specific statistics
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + gsnL3     : UmtsLayer3Gsn *  : Pointer to UMTS GSN Layer3 data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3SgsnPrintStats(Node *node,
                              UmtsLayer3Data* umtsL3,
                              UmtsLayer3Gsn *gsnL3)
{
    char buff[MAX_STRING_LENGTH];

    // GTP tunneled data packet statistics
    sprintf(buff,
            "Number of GTP tunneled packets forwarded to GGSN = %d",
            gsnL3->sgsnStat.numTunnelDataPacketFwdToGgsn);
    IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

    sprintf(buff,
            "Number of GTP tunneled packets forwarded to UE = %d",
            gsnL3->sgsnStat.numTunnelDataPacketFwdToUe);
    IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

    sprintf(buff,
            "Number of GTP tunneled packets dropped = %d",
            gsnL3->sgsnStat.numTunnelDataPacketDropped);
    IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

    //CS domain Packet statistics
    sprintf(buff,
            "Number of CS domain packets forwarded to GGSN = %d",
            gsnL3->sgsnStat.numCsPktsToGmsc);
    IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

    sprintf(buff,
            "Number of CS domain packets forwarded to UE = %d",
            gsnL3->sgsnStat.numCsPktsToUe);
    IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

    sprintf(buff,
            "Number of CS domain packets dropped = %d",
            gsnL3->sgsnStat.numCsPktsDropped);
    IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

    // GMM related statistics
    sprintf(buff,
            "Number of ROUTING AREA UPDATE messages received = %d",
            gsnL3->sgsnStat.numRoutingUpdateRcvd);
    IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

    sprintf(buff,
            "Number of ROUTING AREA UPDATE ACCEPT messages sent = %d",
            gsnL3->sgsnStat.numRoutingUpdateAcptSent);
    IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

    sprintf(buff,
            "Number of ATTACH REQUEST messages received = %d",
            gsnL3->sgsnStat.numAttachReqRcvd);
    IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

    sprintf(buff,
            "Number of ATTACH ACCEPT messages sent = %d",
            gsnL3->sgsnStat.numAttachAcptSent);
    IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

    sprintf(buff,
            "Number of ATTACH REJECT messages sent = %d",
            gsnL3->sgsnStat.numAttachRejSent);
    IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

    sprintf(buff,
            "Number of ATTACH COMPLETE messages received = %d",
            gsnL3->sgsnStat.numAttachComplRcvd);
    IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

    sprintf(buff,
            "Number of PAGING messages sent = %d",
            gsnL3->sgsnStat.numPagingSent);
    IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

    sprintf(buff,
            "Number of SERVICE REQUEST messages received = %d",
            gsnL3->sgsnStat.numServiceReqRcvd);
    IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

    sprintf(buff,
            "Number of SERVICE ACCEPT messages sent = %d",
            gsnL3->sgsnStat.numServiceAcptSent);
    IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

    sprintf(buff,
            "Number of SERVICE REJECT messages sent = %d",
            gsnL3->sgsnStat.numServiceRejSent);
    IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

    // VLR & HLR related statistics
    sprintf(buff,
            "Number of VLR entries added = %d",
            gsnL3->sgsnStat.numVlrEntryAdded);
    IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

    sprintf(buff,
            "Number of VLR entries removed = %d",
            gsnL3->sgsnStat.numVlrEntryRemoved);
    IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

    sprintf(buff,
            "Number of HLR updates performed = %d",
            gsnL3->sgsnStat.numHlrUpdate);
    IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

    if (umtsL3->printDetailedStat)
    {
        // SM with UE
        // PDP Context Activation
        sprintf(buff,
           "Number of ACTIVATE PDP CONTEXT REQUEST messages received = %d",
                gsnL3->sgsnStat.numActivatePDPContextReqRcvd);
        IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of ACTIVATE PDP CONTEXT ACCEPT messages sent = %d",
                gsnL3->sgsnStat.numActivatePDPContextAcptSent);
        IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of ACTIVATE PDP CONTEXT REJECT messages sent = %d",
                gsnL3->sgsnStat.numActivatePDPContextRjtSent);
        IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

        sprintf(buff,
          "Number of REQUEST PDP CONTEXT ACTIVATION messages sent = %d",
                gsnL3->sgsnStat.numReqActivatePDPContextSent);
        IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

        // PDP Context Deactivation
        sprintf(buff,
         "Number of DEACTIVATE PDP CONTEXT REQUEST messages received = %d",
                gsnL3->sgsnStat.numDeactivatePDPContextReqRcvd);
        IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

        sprintf(buff,
           "Number of DEACTIVATE PDP CONTEXT ACCEPT messages sent = %d",
                gsnL3->sgsnStat.numDeactivatePDPContextAcptSent);
        IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

        sprintf(buff,
          "Number of DEACTIVATE PDP CONTEXT REQUEST messages sent = %d",
                gsnL3->sgsnStat.numDeactivatePDPContextReqSent);
        IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

        sprintf(buff,
          "Number of DEACTIVATE PDP CONTEXT ACCEPT messages received = %d",
                gsnL3->sgsnStat.numDeactivatePDPContextAcptRcvd);
        IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

        // PDP Context with GGSN
        // Create PDP Context
        sprintf(buff,
                "Number of CREATE PDP CONTEXT REQUEST messages sent = %d",
                gsnL3->sgsnStat.numCreatePDPContextReqSent);
        IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

        sprintf(buff,
          "Number of CREATE PDP CONTEXT RESPONSE messages received = %d",
                gsnL3->sgsnStat.numCreatePDPContextRspRcvd);
        IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

        sprintf(buff,
           "Number of PDU NOTIFICATION REQUEST messages received = %d",
                gsnL3->sgsnStat.numPduNotificationReqRcvd);
        IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of PDU NOTIFICATION RESPONSE messages sent = %d",
                gsnL3->sgsnStat.numPduNotificationRspSent);
        IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

        // Delete PDP Context
        sprintf(buff,
                "Number of DELETE PDP CONTEXT REQUEST messages sent = %d",
                gsnL3->sgsnStat.numDeletePDPContextReqSent);
        IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

        sprintf(buff,
          "Number of DELETE PDP CONTEXT RESPONSE messages received = %d",
                gsnL3->sgsnStat.numDeletePDPContextRspRcvd);
        IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

        sprintf(buff,
           "Number of DELETE PDP CONTEXT REQUEST messages received = %d",
                gsnL3->sgsnStat.numDeletePDPContextReqRcvd);
        IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of DELETE PDP CONTEXT RESPONSE messages sent = %d",
                gsnL3->sgsnStat.numDeletePDPContextRspSent);
        IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

        //CC messages
        sprintf(buff,
                "Number of call control SETUP messages sent = %d",
                gsnL3->sgsnStat.numCcSetupSent);
        IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of call control CALL CONFIRM "
                "messages received = %d",
                gsnL3->sgsnStat.numCcCallConfirmRcvd);
        IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of call control ALERTING "
                "messages received = %d",
                gsnL3->sgsnStat.numCcAlertingRcvd);
        IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of call control CONNECT "
                "messages received = %d",
                gsnL3->sgsnStat.numCcConnectRcvd);
        IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of call control CONNECT ACKNOWLEDGE "
                "messages sent = %d",
                gsnL3->sgsnStat.numCcConnAckSent);
        IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of call control DISCONNECT "
                "messages sent = %d",
                gsnL3->sgsnStat.numCcDisconnectSent);
        IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of call control RELEASE "
                "messages received = %d",
                gsnL3->sgsnStat.numCcReleaseRcvd);
        IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of call control RELEASE COMPLETE "
                "messages sent = %d",
                gsnL3->sgsnStat.numCcReleaseCompleteSent);
        IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of call control SETUP "
                "messages received = %d",
                gsnL3->sgsnStat.numCcSetupRcvd);
        IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of call control CALL PROCEEDING "
                "messages sent = %d",
                gsnL3->sgsnStat.numCcCallProcSent);
        IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of call control ALERTING"
                "messages sent = %d",
                gsnL3->sgsnStat.numCcAlertingSent);
        IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of call control CONNECT "
                "messages sent = %d",
                gsnL3->sgsnStat.numCcConnectSent);
        IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of call control CONNECT ACKNOWLEDGE "
                "messages received = %d",
                gsnL3->sgsnStat.numCcConnAckRcvd);
        IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of call control DISCONNECT "
                "messages received = %d",
                gsnL3->sgsnStat.numCcDisconnectRcvd);
        IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of call control RELEASE "
                "messages sent = %d",
                gsnL3->sgsnStat.numCcReleaseSent);
        IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of call control RELEASE COMPLETE "
                "messages received = %d",
                gsnL3->sgsnStat.numCcReleaseCompleteRcvd);
        IO_PrintStat(node, "Layer3", "UMTS SGSN", ANY_DEST, -1, buff);
    }

    return;
}

// /**
// FUNCTION   :: UmtsLayer3GgsnPrintStats
// LAYER      :: Layer 3
// PURPOSE    :: Print out GGSN specific statistics
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + gsnL3     : UmtsLayer3Gsn *  : Pointer to UMTS GSN Layer3 data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3GgsnPrintStats(Node *node,
                              UmtsLayer3Data* umtsL3,
                              UmtsLayer3Gsn *gsnL3)
{
    char buff[MAX_STRING_LENGTH];

    // flow related statistics
    // Mobile Terminated Flows
    // Aggregated flow statistics
    sprintf(buff,
            "Number of mobile terminated flows requested = %d",
            gsnL3->ggsnStat.numMtFlowRequested);
    IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

    sprintf(buff,
            "Number of mobile terminated flows admitted = %d",
            gsnL3->ggsnStat.numMtFlowAdmitted);
    IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

    sprintf(buff,
            "Number of mobile terminated flows rejected = %d",
            gsnL3->ggsnStat.numMtFlowRejected);
    IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

    sprintf(buff,
            "Number of mobile terminated flows dropped = %d",
            gsnL3->ggsnStat.numMtFlowDropped);
    IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

    sprintf(buff,
            "Number of mobile terminated flows completed = %d",
            gsnL3->ggsnStat.numMtFlowCompleted);
    IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

    if (umtsL3->printDetailedStat)
    {
        // Mobile Terminated Conversational Flows
        sprintf(buff,
                "Number of mobile terminated conversational "
                "flows requested = %d",
                gsnL3->ggsnStat.numMtConversationalFlowRequested);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of mobile terminated conversational "
                "flows admitted = %d",
                gsnL3->ggsnStat.numMtConversationalFlowAdmitted);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of mobile terminated conversational "
                "flows rejected = %d",
                gsnL3->ggsnStat.numMtConversationalFlowRejected);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of mobile terminated conversational "
                "flows dropped = %d",
                gsnL3->ggsnStat.numMtConversationalFlowDropped);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of mobile terminated conversational "
                "flows completed = %d",
                gsnL3->ggsnStat.numMtConversationalFlowCompleted);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        // Mobile Terminated Streaming Flows
        sprintf(buff,
                "Number of mobile terminated streaming "
                "flows requested = %d",
                gsnL3->ggsnStat.numMtStreamingFlowRequested);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of mobile terminated streaming "
                "flows admitted = %d",
                gsnL3->ggsnStat.numMtStreamingFlowAdmitted);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of mobile terminated streaming "
                "flows rejected = %d",
                gsnL3->ggsnStat.numMtStreamingFlowRejected);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of mobile terminated streaming "
                "flows dropped = %d",
                gsnL3->ggsnStat.numMtStreamingFlowDropped);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of mobile terminated streaming "
                "flows completed = %d",
                gsnL3->ggsnStat.numMtStreamingFlowCompleted);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        // Mobile Terminated Interactive Flows
        sprintf(buff,
                "Number of mobile terminated interactive "
                "flows requested = %d",
                gsnL3->ggsnStat.numMtInteractiveFlowRequested);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of mobile terminated interactive "
                "flows admitted = %d",
                gsnL3->ggsnStat.numMtInteractiveFlowAdmitted);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of mobile terminated interactive "
                "flows rejected = %d",
                gsnL3->ggsnStat.numMtInteractiveFlowRejected);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of mobile terminated interactive "
                "flows dropped = %d",
                gsnL3->ggsnStat.numMtInteractiveFlowDropped);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of mobile terminated interactive "
                "flows completed = %d",
                gsnL3->ggsnStat.numMtInteractiveFlowCompleted);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        // Mobile Terminated Background Flows
        sprintf(buff,
                "Number of mobile terminated background "
                "flows requested = %d",
                gsnL3->ggsnStat.numMtBackgroundFlowRequested);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of mobile terminated background "
                "flows admitted = %d",
                gsnL3->ggsnStat.numMtBackgroundFlowAdmitted);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of mobile terminated background "
                "flows rejected = %d",
                gsnL3->ggsnStat.numMtBackgroundFlowRejected);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of mobile terminated background "
                "flows dropped = %d",
                gsnL3->ggsnStat.numMtBackgroundFlowDropped);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of mobile terminated background "
                "flows completed = %d",
                gsnL3->ggsnStat.numMtBackgroundFlowCompleted);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);
    }

    // Mobile Originated Flows
    // Aggregated flow statistics
    sprintf(buff,
            "Number of mobile originated flows requested = %d",
            gsnL3->ggsnStat.numMoFlowRequested);
    IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

    sprintf(buff,
            "Number of mobile originated flows admitted = %d",
            gsnL3->ggsnStat.numMoFlowAdmitted);
    IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

    sprintf(buff,
            "Number of mobile originated flows rejected = %d",
            gsnL3->ggsnStat.numMoFlowRejected);
    IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

    sprintf(buff,
            "Number of mobile originated flows dropped = %d",
            gsnL3->ggsnStat.numMoFlowDropped);
    IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

    sprintf(buff,
            "Number of mobile originated flows completed = %d",
            gsnL3->ggsnStat.numMoFlowCompleted);
    IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

    if (umtsL3->printDetailedStat)
    {
        // Mobile Originated Conversational Flows
        sprintf(buff,
                "Number of mobile originated conversational "
                "flows requested = %d",
                gsnL3->ggsnStat.numMoConversationalFlowRequested);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of mobile originated conversational "
                "flows admitted = %d",
                gsnL3->ggsnStat.numMoConversationalFlowAdmitted);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of mobile originated conversational "
                "flows rejected = %d",
                gsnL3->ggsnStat.numMoConversationalFlowRejected);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of mobile originated conversational "
                "flows dropped = %d",
                gsnL3->ggsnStat.numMoConversationalFlowDropped);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of mobile originated conversational "
                "flows completed = %d",
                gsnL3->ggsnStat.numMoConversationalFlowCompleted);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        // Mobile Originated Streaming Flows
        sprintf(buff,
                "Number of mobile originated streaming flows "
                "requested = %d",
                gsnL3->ggsnStat.numMoStreamingFlowRequested);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of mobile originated streaming flows "
                "admitted = %d",
                gsnL3->ggsnStat.numMoStreamingFlowAdmitted);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of mobile originated streaming flows "
                "rejected = %d",
                gsnL3->ggsnStat.numMoStreamingFlowRejected);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of mobile originated streaming "
                "flows dropped = %d",
                gsnL3->ggsnStat.numMoStreamingFlowDropped);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of mobile originated streaming "
                "flows completed = %d",
                gsnL3->ggsnStat.numMoStreamingFlowCompleted);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        // Mobile Originated Interactive Flows
        sprintf(buff,
                "Number of mobile originated interactive "
                "flows requested = %d",
                gsnL3->ggsnStat.numMoInteractiveFlowRequested);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of mobile originated interactive "
                "flows admitted = %d",
                gsnL3->ggsnStat.numMoInteractiveFlowAdmitted);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of mobile originated interactive "
                "flows rejected = %d",
                gsnL3->ggsnStat.numMoInteractiveFlowRejected);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of mobile originated interactive flows dropped = %d",
                gsnL3->ggsnStat.numMoInteractiveFlowDropped);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of mobile originated interactive "
                "flows completed = %d",
                gsnL3->ggsnStat.numMoInteractiveFlowCompleted);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        // Mobile Originated Background Flows
        sprintf(buff,
                "Number of mobile originated background "
                "flows requested = %d",
                gsnL3->ggsnStat.numMoBackgroundFlowRequested);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of mobile originated background "
                "flows admitted = %d",
                gsnL3->ggsnStat.numMoBackgroundFlowAdmitted);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of mobile originated background "
                "flows rejected = %d",
                gsnL3->ggsnStat.numMoBackgroundFlowRejected);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of mobile originated background "
                "flows dropped = %d",
                gsnL3->ggsnStat.numMoBackgroundFlowDropped);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of mobile originated background "
                "flows completed = %d",
                gsnL3->ggsnStat.numMoBackgroundFlowCompleted);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);
    }

    // aggregated data packet statistics
    sprintf(buff,
            "Number of PS domain data packets from "
            "outside of my PLMN = %d",
            gsnL3->ggsnStat.numPsDataPacketFromOutsidePlmn);
    IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

    sprintf(buff,
            "Number of PS domain data packets "
            "routed to my PLMN = %d",
            gsnL3->ggsnStat.numPsDataPacketToPlmn);
    IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

    sprintf(buff,
            "Number of PS domain data packets "
            "from my PLMN = %d",
            gsnL3->ggsnStat.numPsDataPacketFromPlmn);
    IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

    sprintf(buff,
            "Number of PS domain data packets "
            "routed to outside of my PLMN = %d",
            gsnL3->ggsnStat.numPsDataPacketToOutsidePlmn);
    IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

    sprintf(buff,
            "Number of PS domain data packets dropped = %d",
            gsnL3->ggsnStat.numPsDataPacketDropped);
    IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

    sprintf(buff,
            "Number of PS domain data packets "
            "dropped (unsupported format)= %d",
            gsnL3->ggsnStat.numPsDataPacketDroppedUnsupportedFormat);
    IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

    // aggregated data packet statistics
    sprintf(buff,
            "Number of CS domain data packets "
            "from outside of my PLMN = %d",
            gsnL3->ggsnStat.numCsPktsFromOutside);
    IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

    sprintf(buff,
            "Number of CS domain data packets "
            "routed to my PLMN = %d",
            gsnL3->ggsnStat.numCsPktsToInside);
    IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

    sprintf(buff,
            "Number of CS domain data packets from my PLMN = %d",
            gsnL3->ggsnStat.numCsPktsFromInside);
    IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

    sprintf(buff,
            "Number of CS domain data packets routed "
            "to outside of my PLMN = %d",
            gsnL3->ggsnStat.numCsPktsToOutside);
    IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

    sprintf(buff,
            "Number of CS domain data packets dropped = %d",
            gsnL3->ggsnStat.numCsPktsDropped);
    IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

    // Statistics for querying HLR for routing information
    sprintf(buff,
            "Number of routing information queries sent to HLR = %d",
            gsnL3->ggsnStat.numRoutingQuerySent);
    IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

    sprintf(buff,
            "Number of routing info replies with success status received = %d",
            gsnL3->ggsnStat.numRoutingQueryReplyRcvd);
    IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

    sprintf(buff,
            "Number of routing info replies "
            "with failure status received = %d",
            gsnL3->ggsnStat.numRoutingQueryFailRcvd);
    IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

    if (umtsL3->printDetailedStat)
    {
        // PDP Context related statistics
        // Create PDP Context
        sprintf(buff,
                "Number of PDU NOTIFICATION REQUEST messages sent = %d",
                gsnL3->ggsnStat.numPduNotificationReqSent);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of PDU NOTIFICATION RESPONSE "
                "messages received = %d",
                gsnL3->ggsnStat.numPduNotificationRspRcvd);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of CREATE PDP CONTEXT REQUEST "
                "messages received = %d",
                gsnL3->ggsnStat.numCreatePDPContextReqRcvd);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of CREATE PDP CONTEXT RESPONSE messages sent = %d",
                gsnL3->ggsnStat.numCreatePDPContextRspSent);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        // Delete PDP Context
        sprintf(buff,
                "Number of DELETE PDP CONTEXT REQUEST "
                "messages received = %d",
                gsnL3->ggsnStat.numDeletePDPContextReqRcvd);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of DELETE PDP CONTEXT RESPONSE messages sent = %d",
                gsnL3->ggsnStat.numDeletePDPContextRspSent);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of DELETE PDP CONTEXT REQUEST messages sent = %d",
                gsnL3->ggsnStat.numDeletePDPContextReqSent);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);

        sprintf(buff,
                "Number of DELETE PDP CONTEXT RESPONSE "
                "messages received = %d",
                gsnL3->ggsnStat.numDeletePDPContextRspRcvd);
        IO_PrintStat(node, "Layer3", "UMTS GGSN", ANY_DEST, -1, buff);
    }

    return;
}


//////////////
// GSN APP
/////////////
// /**
// FUNCTION   :: UmtsLayer3GsnAddAppInfoToList
// LAYER      :: Layer 3
// PURPOSE    :: Configurate the CCCH, RACH and PRACH for random access
// + node      : Node*             : Pointer to node.
// + gsnL3     : UmtsLayer3Gsn*    : GSN Layer3 data
// + appInfoList : std::list<UmtsLayer3SgsnFlowInfo*>* : Application list
// + appClsPt  : UmtsLayer3FlowClassifier* : classifier
// + srcDest   : UmtsFlowSrcDestType  : applicaiton src dest type
// + transId   : unsigned char     : transactionId
// + naspi     : unsigned char     :  NSAPI
// RETURN     :: UmtsLayer3UeAppInfo* : newly added appInfo
// **/
static
UmtsLayer3SgsnFlowInfo* UmtsLayer3GsnAddAppInfoToList(
                          Node* node,
                          UmtsLayer3Gsn* gsnL3,
                          std::list<UmtsLayer3SgsnFlowInfo*>* appInfoList,
                          UmtsLayer3FlowClassifier* appClsPt,
                          UmtsFlowSrcDestType srcDestType,
                          unsigned char transId,
                          unsigned char nsapi = 0)
{
    UmtsLayer3SgsnFlowInfo* appInfo;

    appInfo = new UmtsLayer3SgsnFlowInfo(transId, srcDestType);

    // incldue the classifier info
    appInfo->classifierInfo =
            (UmtsLayer3FlowClassifier*)MEM_malloc(
            sizeof(UmtsLayer3FlowClassifier));

    memcpy(appInfo->classifierInfo,
            appClsPt,
            sizeof(UmtsLayer3FlowClassifier));

    if (appInfo->classifierInfo->domainInfo == UMTS_LAYER3_CN_DOMAIN_CS)
    {
        UmtsLayer3GsnInitUeCc(node, gsnL3, appInfo);
    }
    else
    {
        appInfo->cmData.smData =
            (UmtsSmNwData*) MEM_malloc(sizeof(UmtsSmNwData));
        memset((void*)(appInfo->cmData.smData), 0, sizeof(UmtsSmNwData));
        if (srcDestType == UMTS_FLOW_MOBILE_ORIGINATED)
        {
            appInfo->cmData.smData->smState = UMTS_SM_NW_PDP_ACTIVE_PENDING;
        }
        else if (srcDestType == UMTS_FLOW_NETWORK_ORIGINATED)
        {
            appInfo->cmData.smData->smState = UMTS_SM_NW_PDP_INACTIVE;
        }
        appInfo->nsapi = nsapi;
        appInfo->ulTeId = (UInt32) -1;
    }

    // QoS info
    appInfo->qosInfo =
        (UmtsFlowQoSInfo*)MEM_malloc(sizeof(UmtsFlowQoSInfo));
    memset((void*)(appInfo->qosInfo), 0, sizeof(UmtsFlowQoSInfo));

    // TODO: figure out the right GGSN for this PDP
    appInfo->ggsnAddr = gsnL3->ggsnAddr;
    appInfo->ggsnNodeId = gsnL3->ggsnNodeId;

    // get the OQs info from Request
    // put into the list
    appInfoList->push_back(appInfo);

    return appInfo;
}

// /**
// FUNCTION   :: UmtsLayer3GsnRemoveAppInfoFromList
// LAYER      :: Layer 3
// PURPOSE    :: Configurate the CCCH, RACH and PRACH for random access
// + node      : Node*             : Pointer to node.
// + gsnL3     : UmtsLayer3Gsn*    : GSN Layer3 data
// + transIdBitSet : std::bitset<UMTS_MAX_TRANS_MOBILE>* : Trans. Id set
// + appInfoList : std::list<UmtsLayer3SgsnFlowInfo*>* : Application list
// + srcDest   : UmtsFlowSrcDestType  : applicaiton src dest type
// + transId   : unsigned char     : transactionId
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3GsnRemoveAppInfoFromList(
           Node* node,
           UmtsLayer3Gsn* gsnL3,
           std::bitset<UMTS_MAX_TRANS_MOBILE>* transIdBitSet,
           std::list<UmtsLayer3SgsnFlowInfo*>* appInfoList,
           UmtsFlowSrcDestType srcDestType,
           unsigned char transId)
{
    // when calling this fucniton
    // needs to clear the bitset for this transId  if the app
    // is UMTS_FLOW_NETOWRK_ORIGINATED
    std::list<UmtsLayer3SgsnFlowInfo*>::iterator it;

    for (it = appInfoList->begin();
        it != appInfoList->end();
        it ++)
    {
        if ((*it)->transId == transId &&
            (*it)->srcDestType == srcDestType)
        {
            break;
        }
    }

    //ERROR_Assert(it != appInfoList->end(),
    //             " No appInfo find in the lsit link "
    //             "with the transId and srcDestType");
    if (it == appInfoList->end())
    {
        return;
    }

   // reset bit set
    if (srcDestType == UMTS_FLOW_NETWORK_ORIGINATED)
    {
        transIdBitSet->reset(transId - UMTS_LAYER3_NETWORK_MOBILE_START);
    }
    UmtsLayer3GsnAppInfoFinalize(node,
                                 gsnL3,
                                 (*it));
    appInfoList->erase(it);
}

// /**
// FUNCTION   :: UmtsLayer3GsnFindAppInfoFromList
// LAYER      :: Layer 3
// PURPOSE    :: Configurate the CCCH, RACH and PRACH for random access
// + node      : Node*             : Pointer to node.
// + gsnL3     : UmtsLayer3Gsn*    : GSN Layer3 data
// + appInfoList : std::list<UmtsLayer3SgsnFlowInfo*>* : Application list
// + srcDest   : UmtsFlowSrcDestType  : applicaiton src dest type
// + transId   : unsigned char     : transactionId
// RETURN     :: UmtsLayer3UeAppInfo* : found appInfo
// **/
static
UmtsLayer3SgsnFlowInfo* UmtsLayer3GsnFindAppInfoFromList(
                         Node* node,
                         UmtsLayer3Gsn* gsnL3,
                         std::list<UmtsLayer3SgsnFlowInfo*>* appInfoList,
                         UmtsFlowSrcDestType srcDestType,
                         unsigned char transId)
{
    std::list<UmtsLayer3SgsnFlowInfo*>::iterator it;

    for (it = appInfoList->begin();
        it != appInfoList->end();
        it ++)
    {
        if ((*it)->transId == transId &&
            (*it)->srcDestType == srcDestType)
        {
            break;
        }
    }

    if (it != appInfoList->end())
    {
        return (*it);
    }
    else
    {
        return (UmtsLayer3SgsnFlowInfo*)NULL;
    }
}

// /**
// FUNCTION   :: UmtsLayer3GsnFindAppInfoFromList
// LAYER      :: Layer 3
// PURPOSE    :: Find application information from the UE application list
// + node      : Node*             : Pointer to node.
// + gsnL3     : UmtsLayer3Gsn*    : GSN Layer3 data
// + ueNas     : UmtsSgsnUeDataInSgsn* : UE data in GSN
// + transId   : unsigned char     : transactionId
// RETURN     :: UmtsLayer3UeAppInfo* : found appInfo
// **/
static
UmtsLayer3SgsnFlowInfo* UmtsLayer3GsnFindAppInfoFromList(
                         Node* node,
                         UmtsLayer3Gsn* gsnL3,
                         UmtsSgsnUeDataInSgsn* ueNas,
                         unsigned char transId)
{
    std::list<UmtsLayer3SgsnFlowInfo*>::iterator it;

    for (it = ueNas->appInfoList->begin();
         it != ueNas->appInfoList->end();
         it++)
    {
        if ((*it)->transId == transId)
        {
            break;
        }
    }

    if (it != ueNas->appInfoList->end())
    {
        return (*it);
    }
    else
    {
        return (UmtsLayer3SgsnFlowInfo*)NULL;
    }
}

#if 0
// /**
// FUNCTION   :: UmtsLayer3GsnFindAppInfoFromList
// LAYER      :: Layer 3
// PURPOSE    :: Configurate the CCCH, RACH and PRACH for random access
// + node      : Node*             : Pointer to node.
// + gsnL3     : UmtsLayer3Gsn*    : GSN Layer3 data
// + appInfoList : std::list<UmtsLayer3SgsnFlowInfo*>* : Application list
// + appClsPt  : UmtsLayer3FlowClassifier* : pointer to the classifier info
// RETURN     :: UmtsLayer3UeAppInfo* : found appInfo
// **/
static
UmtsLayer3SgsnFlowInfo* UmtsLayer3GsnFindAppInfoFromList(
                         Node* node,
                         UmtsLayer3Gsn* gsnL3,
                         std::list<UmtsLayer3SgsnFlowInfo*>* appInfoList,
                         UmtsLayer3FlowClassifier* appClsPt)
{
    std::list<UmtsLayer3SgsnFlowInfo*>::iterator it;

    for (it = appInfoList->begin();
        it != appInfoList->end();
        it ++)
    {

        if (memcmp((void*)(((*it)->classifierInfo)),
                   (void*)(appClsPt),
                   sizeof(UmtsLayer3FlowClassifier)) == 0)
        {
            break;
        }
    }

    if (it != appInfoList->end())
    {
        return (*it);
    }
    else
    {
        return (UmtsLayer3SgsnFlowInfo*)NULL;
    }
}
#endif

// /**
// FUNCTION   :: UmtsLayer3GsnFindAppInfoFromList
// LAYER      :: Layer 3
// PURPOSE    :: Find flow info from the list for given GTP tunnel ID
// + node      : Node*          : Pointer to node.
// + gsnL3     : UmtsLayer3Gsn* : Pointer to GSN node data
// + teId      : UInt32         : Tunnel ID, Uplink if uplink is TRUE
// + uplink    : BOOL           : Whether packet is going uplink direction
// RETURN     :: UmtsLayer3SgsnFlowInfo* : Pointer to flow if found
//                                         NULL if not found
// **/
static
UmtsLayer3SgsnFlowInfo* UmtsLayer3GsnFindAppInfoFromList(
                            Node* node,
                            UmtsLayer3Gsn* gsnL3,
                            UInt32 teId,
                            BOOL uplink)
{
    std::list<UmtsSgsnUeDataInSgsn*>::iterator itUe;
    std::list<UmtsLayer3SgsnFlowInfo*>::iterator itFlow;
    UmtsLayer3SgsnFlowInfo* flowInfo = NULL;

    // find out the pending flows
    for (itUe = gsnL3->ueInfoList->begin();
         itUe != gsnL3->ueInfoList->end();
         itUe ++)
    {
        // check if the flow already exist
        for (itFlow = (*itUe)->appInfoList->begin();
            itFlow != (*itUe)->appInfoList->end();
            itFlow ++)
        {
            if ((uplink && (*itFlow)->ulTeId == teId) ||
                (!uplink && (*itFlow)->dlTeId == teId))
            {
                flowInfo = (*itFlow);
                break;
            }
        }

        if (flowInfo != NULL)
        {
            break;
        }
    }

    return flowInfo;
}

// /**
// FUNCTION   :: UmtsLayer3GsnFindAppByRabId
// LAYER      :: Layer 3
// PURPOSE    :: Find flow info from the list for given GTP tunnel ID
// + node      : Node*          : Pointer to node.
// + gsnL3     : UmtsLayer3Gsn* : Pointer to GSN node data
// + ueNas     : UmtsSgsnUeDataInSgsn* : UE data in GSN
// + rabId     : char           : Rab Id
// RETURN     :: UmtsLayer3SgsnFlowInfo* : Pointer to flow if found
//                                         NULL if not found
// **/
static
UmtsLayer3SgsnFlowInfo* UmtsLayer3GsnFindAppByRabId(
                            Node* node,
                            UmtsLayer3Gsn* gsnL3,
                            UmtsSgsnUeDataInSgsn* ueNas,
                            char   rabId)
{
    std::list<UmtsLayer3SgsnFlowInfo*>::iterator ueFlowIter;
    ueFlowIter =
        find_if(ueNas->appInfoList->begin(),
                ueNas->appInfoList->end(),
                UmtsFindItemByRabId<UmtsLayer3SgsnFlowInfo>(rabId));
    if (ueFlowIter != ueNas->appInfoList->end())
    {
        return *ueFlowIter;
    }
    else
    {
        return NULL;
    }
}

// /**
// FUNCTION   :: UmtsLayer3MscFindVoiceCallFlow
// LAYER      :: Layer 3
// PURPOSE    :: Find a voice call flow info
// + node      : Node*             : Pointer to node.
// + gsnL3     : UmtsLayer3Gsn*    : Pointer to gsn
// + ueNas     : UmtsSgsnUeDataInSgsn* : UE data in GSN
// + peerAddr  : NodeId            : Peer address
// + callAppId : int               : app Id of the call
// RETURN     :: UmtsLayer3UeAppInfo* : found appInfo
// **/
static
UmtsLayer3SgsnFlowInfo* UmtsLayer3MscFindVoiceCallFlow(
                         Node* node,
                         UmtsLayer3Gsn* gsnL3,
                         UmtsSgsnUeDataInSgsn* ueNas,
                         NodeId peerAddr,
                         int    callAppId)
{
    std::list<UmtsLayer3SgsnFlowInfo*>::iterator it;

    for (it = ueNas->appInfoList->begin();
         it != ueNas->appInfoList->end();
         it++)
    {
        if ((*it)->classifierInfo->domainInfo == UMTS_LAYER3_CN_DOMAIN_CS
            &&(*it)->cmData.ccData)
        {
            UmtsCcNwData* ueCc = (*it)->cmData.ccData;
            if (ueCc->peerAddr == peerAddr
                && ueCc->callAppId == callAppId)
                return (*it);
            break;
        }
    }
    return NULL;
}

/////////////
// MM
//////////////
// /**
// FUNCTION   :: UmtsLayer3SgsnSendPaging
// LAYER      :: Layer 3
// PURPOSE    :: Send a Paging message to one RNC
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + ueId      : NodeId                 : UE ID
// + domain    : UmtsLayer3CnDomainId   : CN domain ID
// + cause     : UmtsPagingCause        : Paging cause
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3SgsnSendPaging(
        Node *node,
        UmtsLayer3Data *umtsL3,
        NodeId ueId,
        UmtsLayer3CnDomainId domain,
        UmtsPagingCause cause)
{
    UmtsLayer3Gsn* gsnL3 = (UmtsLayer3Gsn*) umtsL3->dataPtr;

    NodeId rncId;
    UInt32 cellId;
    BOOL found = UmtsLayer3GsnLookupVlr(node,
                                        umtsL3,
                                        ueId,
                                        &rncId,
                                        &cellId);
    if (!found)
    {
        return;
    }

    int packetSize = sizeof(ueId) + 2;
    Message* msg =
        MESSAGE_Alloc(node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        msg,
                        packetSize,
                        TRACE_UMTS_LAYER3);

    char* packet = MESSAGE_ReturnPacket(msg);
    memcpy(packet, &ueId, sizeof(ueId));
    packet += sizeof(ueId);
    (*packet++) = (char) domain;
    (*packet) = (char) cause;

    UmtsLayer3SgsnSendRanapMsgToRnc(
        node,
        umtsL3,
        msg,
        ueId,
        UMTS_RANAP_MSG_TYPE_PAGING,
        rncId);

    // update statistics
    gsnL3->sgsnStat.numPagingSent ++;
}

// /**
// FUNCTION   :: UmtsLayer3SgsnSendLocationUpdateAccept
// LAYER      :: Layer 3
// PURPOSE    :: Send a Paging message to one RNC
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + ueId      : NodeId                 : UE ID
// + updateResult : unsigned char       : result of the loc update
// + raUpTime  : unsinged char          : update time
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3SgsnSendLocationUpdateAccept(
        Node *node,
        UmtsLayer3Data *umtsL3,
        NodeId ueId,
        unsigned char updateResult,
        unsigned char raUpTime)
{
    UmtsLayer3Gsn* gsnL3 = (UmtsLayer3Gsn*) umtsL3->dataPtr;

    UmtsLayer3RoutingUpdateAccept raUpAcpt;

    // fill the contents
    raUpAcpt.periodicRaUpTime = raUpTime;

    UmtsVlrEntry* vlrEntry =  gsnL3->vlrMap->find(ueId)->second;
    raUpAcpt.rncId = vlrEntry->rncId;
    memcpy(&raUpAcpt.newRai, &vlrEntry->rai, sizeof(UmtsRoutingAreaId));

    UmtsLayer3GsnSendNasMsg(
        node,
        umtsL3,
        (char*)&raUpAcpt,
        sizeof(raUpAcpt),
        (char)UMTS_LAYER3_PD_MM,
        (char)UMTS_MM_MSG_TYPE_LOCATION_UPDATING_ACCEPT,
        0,
        ueId);

    // update statistics
    gsnL3->sgsnStat.numRoutingUpdateAcptSent ++;

    if (DEBUG_MM)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_MM);
        printf("%d send a loc up accept to ueId %d\n",node->nodeId, ueId);
    }
}

// /**
// FUNCTION   :: UmtsLayer3SgsnSendCmServAcpt
// LAYER      :: Layer 3
// PURPOSE    :: Send a CM SERVICE ACCEPT to one RNC
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + ueNas     : UmtsSgsnUeDataInSgsn* : UE data in GSN
// + pd        : UmtsLayer3PD           : Protocol discriminator
// + ti        : unsigned char          : transaction Id
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3SgsnSendCmServAcpt(
        Node *node,
        UmtsLayer3Data *umtsL3,
        UmtsSgsnUeDataInSgsn* ueNas,
        UmtsLayer3PD pd,
        unsigned char ti)
{
    char pktBuff[sizeof(pd)];
    memcpy(&pktBuff, &pd, sizeof(pd));

    UmtsLayer3GsnSendNasMsg(
        node,
        umtsL3,
        pktBuff,
        sizeof(pd),
        (char)UMTS_LAYER3_PD_MM,
        (char)UMTS_MM_MSG_TYPE_CM_SERVICE_ACCEPT,
        ti,
        ueNas->tmsi);
}

// /**
// FUNCTION   :: UmtsLayer3SgsnSendIuRelease
// LAYER      :: Layer 3
// PURPOSE    :: Send an IU_RELEASE_COMMAND to the RNC
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + ueId      : NodeId                 : UE ID
// + domain    : UmtsLayer3CnDomainId   : Domain Id
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3SgsnSendIuRelease(
        Node *node,
        UmtsLayer3Data *umtsL3,
        NodeId ueId,
        UmtsLayer3CnDomainId domain)
{
    NodeId rncId;
    UInt32 cellId;
    if (TRUE == UmtsLayer3GsnLookupVlr(
                    node,
                    umtsL3,
                    ueId,
                    &rncId,
                    &cellId))
    {
        Message* msg =
            MESSAGE_Alloc(node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);
        MESSAGE_PacketAlloc(node,
                            msg,
                            sizeof(char),
                            TRACE_UMTS_LAYER3);
        char* packet = MESSAGE_ReturnPacket(msg);
        *packet = (char)domain;

        UmtsLayer3SgsnSendRanapMsgToRnc(
            node,
            umtsL3,
            msg,
            ueId,
            UMTS_RANAP_MSG_TYPE_IU_RELEASE_COMMAND,
            rncId);
    }
}

///////////////
/// GTP
//////////////
// /**
// FUNCTION   :: UmtsLayer3SgsnSendGtpRabAssgt
// LAYER      :: Layer 3
// PURPOSE    :: Send a RAB ASSIGNMENT REQUEST to one RNC for establishing
//               a GTP tunnelling
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + ueId      : NodeId                 : UE ID
// + connMap: UmtsRabConnMap*        : RAB connection map
// + ulrabPara : UmtsRABServiceInfo*    : UL RAB QoS requirment parameters
// + dlrabPara : UmtsRABServiceInfo*    : DL RAB QoS requirment parameters
// + rncId     : NodeId                 : The RNC ID the msg will be sent to
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3SgsnSendGtpRabAssgt(
        Node *node,
        UmtsLayer3Data *umtsL3,
        NodeId ueId,
        const UmtsRabConnMap* connMap,
        const UmtsRABServiceInfo* ulRabPara,
        const UmtsRABServiceInfo* dlRabPara,
        NodeId rncId)
{
    std::string msgBuf;
    msgBuf += (char)UMTS_LAYER3_CN_DOMAIN_PS;
    msgBuf.append((char*)connMap, sizeof(UmtsRabConnMap));

    // uplink first, then down link
    msgBuf.append((char*)ulRabPara, sizeof(UmtsRABServiceInfo));
    msgBuf.append((char*)dlRabPara, sizeof(UmtsRABServiceInfo));

    Message* msg =
        MESSAGE_Alloc(node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        msg,
                        (int)msgBuf.size(),
                        TRACE_UMTS_LAYER3);
    memcpy(MESSAGE_ReturnPacket(msg),
           msgBuf.data(),
           msgBuf.size());
    UmtsLayer3SgsnSendRanapMsgToRnc(
        node,
        umtsL3,
        msg,
        ueId,
        UMTS_RANAP_MSG_TYPE_RAB_ASSIGNMENT_REQUEST,
        rncId);
}

// FUNCTION   :: UmtsLayer3MscSendVoiceCallRabAssgt
// LAYER      :: Layer 3
// PURPOSE    :: Send a RAB ASSIGNMENT REQUEST to one RNC for establishing
//               a voice call virtual circuit
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + ueId      : NodeId                 : UE ID
// + ti        : char                   : transaction Id
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3MscSendVoiceCallRabAssgt(
        Node *node,
        UmtsLayer3Data *umtsL3,
        NodeId ueId,
        char   ti)
{
    NodeId rncId;
    NodeId cellId;

    if (!UmtsLayer3GsnLookupVlr(node, umtsL3, ueId, &rncId, &cellId))
    {
        return;
    }

    std::string msgBuf;
    msgBuf += (char)UMTS_LAYER3_CN_DOMAIN_CS;

    UmtsRabConnMap connMap;
    connMap.flowMap.pd = UMTS_LAYER3_PD_CC;
    connMap.flowMap.ti = ti;
    msgBuf.append((char*)&connMap, sizeof(UmtsRabConnMap));

    Message* msg =
        MESSAGE_Alloc(node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        msg,
                        (int)msgBuf.size(),
                        TRACE_UMTS_LAYER3);
    memcpy(MESSAGE_ReturnPacket(msg),
           msgBuf.data(),
           msgBuf.size());
    UmtsLayer3SgsnSendRanapMsgToRnc(
        node,
        umtsL3,
        msg,
        ueId,
        UMTS_RANAP_MSG_TYPE_RAB_ASSIGNMENT_REQUEST,
        rncId);
}

// /**
// FUNCTION   :: UmtsLayer3SgsnSendGtpRabRelease
// LAYER      :: Layer 3
// PURPOSE    :: Send a RAB ASSIGNMENT Release to one RNC to release
//               a RAB
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + ueId      : NodeId                 : UE ID
// + rabId     : char                   : RAB ID to be released
// + rncId     : NodeId    : The RNC ID the message will be sent to
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3SgsnSendGtpRabRelease(
        Node *node,
        UmtsLayer3Data *umtsL3,
        NodeId ueId,
        char rabId,
        NodeId rncId)
{
    std::string msgBuf;
    msgBuf += rabId;

    Message* msg =
        MESSAGE_Alloc(node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        msg,
                        (int)msgBuf.size(),
                        TRACE_UMTS_LAYER3);
    memcpy(MESSAGE_ReturnPacket(msg),
           msgBuf.data(),
           msgBuf.size());
    UmtsLayer3SgsnSendRanapMsgToRnc(
        node,
        umtsL3,
        msg,
        ueId,
        UMTS_RANAP_MSG_TYPE_RAB_ASSIGNMENT_RELEASE,
        rncId);
}

// /**
// FUNCTION   :: UmtsLayer3MscSendVoiceCallRabRelease
// LAYER      :: Layer 3
// PURPOSE    :: Send a RAB ASSIGNMENT Release to one RNC to release
//               a RAB
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + ueId      : NodeId                 : UE ID
// + rabId     : char                   : RAB ID to be released
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3MscSendVoiceCallRabRelease(
    Node *node,
    UmtsLayer3Data *umtsL3,
    NodeId ueId,
    char rabId)
{
    NodeId rncId;
    NodeId cellId;
    BOOL found = UmtsLayer3GsnLookupVlr(node,
                                        umtsL3,
                                        ueId,
                                        &rncId,
                                        &cellId);
    if (found)
    {
        Message* msg =
            MESSAGE_Alloc(node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);
        MESSAGE_PacketAlloc(node,
                            msg,
                            sizeof(char),
                            TRACE_UMTS_LAYER3);
        (*MESSAGE_ReturnPacket(msg)) = rabId;
        UmtsLayer3SgsnSendRanapMsgToRnc(
            node,
            umtsL3,
            msg,
            ueId,
            UMTS_RANAP_MSG_TYPE_RAB_ASSIGNMENT_RELEASE,
            rncId);
    }
}

// /**
// FUNCTION   :: UmtsLayer3GsnGtpSendCreatePDPContextRequest
// LAYER      :: Layer 3
// PURPOSE    :: Send a RAB ASSIGNMENT Release to one RNC for establishing
//               a GTP tunnelling
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + gsnL3     : UmtsLayer3Gsn*         : GSN data
// + appInfo   : UmtsLayer3SgsnFlowInfo* : applicaiton info
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3GsnGtpSendCreatePDPContextRequest(
          Node*node,
          UmtsLayer3Data* umtsL3,
          UmtsLayer3Gsn* gsnL3,
          UmtsLayer3SgsnFlowInfo* appInfo)
{
    char pktBuff[UMTS_MAX_PACKET_BUFFER_SIZE];
    UmtsGtpCreatePDPContextRequest *pdpReqMsg;
    int index = 0;
    Message* msg;

    memset(pktBuff, 0, UMTS_MAX_PACKET_BUFFER_SIZE);
    pdpReqMsg = (UmtsGtpCreatePDPContextRequest *) &(pktBuff[0]);
    index += sizeof(UmtsGtpCreatePDPContextRequest);

    // fill the contents
    pdpReqMsg->dlTeId = appInfo->dlTeId;
    pdpReqMsg->nsapi = appInfo->nsapi;
    pdpReqMsg->trafficClass = appInfo->qosInfo->trafficClass;

    // create the msg
    msg = MESSAGE_Alloc(node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        msg,
                        index,
                        TRACE_UMTS_LAYER3);

    memcpy((char*)MESSAGE_ReturnPacket(msg),
            pktBuff, index);

    // TODO: set the first byte of the gtp header
    unsigned char firstByte = UMTS_GTP_HEADER_FIRST_BYTE;
    UmtsAddGtpHeader(node, msg, firstByte,
                     UMTS_GTP_CREATE_PDP_CONTEXT_REQUEST,
                     appInfo->ulTeId, NULL, 0);
    // this pdpReqMsg->dlTeId put in the GTP header
    // should not be used for other purpose
    UmtsLayer3GsnSendGtpMsgOverBackbone(
        node, msg, appInfo->ggsnAddr);

    if ((DEBUG_GTP || DEBUG_SM) && appInfo->ueId == DEBUG_UE_ID)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_SM);
        printf("\n\tSend CREATE PDP CONTEXT REQUEST to GGSN %d for "
               "NSAPI %d with DL TeId %d\n",
               appInfo->ggsnNodeId, appInfo->nsapi, appInfo->dlTeId);
    }
}

// /**
// FUNCTION   :: UmtsLayer3GsnGtpSendCreatePDPContextResponse
// LAYER      :: Layer 3
// PURPOSE    :: Send a RAB ASSIGNMENT Release to one RNC for establishing
//               a GTP tunnelling
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + gsnL3     : UmtsLayer3Gsn*         : GSN data
// + pdpInfo   : UmtsLayer3GgsnPdpInfo* : applicaiton info
// + gtpCause  : UmtsGtpCauseType       : GTP cause
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3GsnGtpSendCreatePDPContextResponse(
          Node*node,
          UmtsLayer3Data* umtsL3,
          UmtsLayer3Gsn* gsnL3,
          UmtsLayer3GgsnPdpInfo* pdpInfo,
          UmtsGtpCauseType gtpCause)
{
    char pktBuff[UMTS_MAX_PACKET_BUFFER_SIZE];
    UmtsGtpCreatePDPContextResponse *pdpRspMsg;
    int index = 0;
    Message* msg;

    memset(pktBuff, 0, UMTS_MAX_PACKET_BUFFER_SIZE);
    pdpRspMsg = (UmtsGtpCreatePDPContextResponse *) &(pktBuff[0]);
    index += sizeof(UmtsGtpCreatePDPContextResponse);

    // fill the contents
    pdpRspMsg->nsapi = pdpInfo->nsapi;
    pdpRspMsg->ulTeId = pdpInfo->ulTeId;
    pdpRspMsg->gtpCause = (unsigned char) gtpCause;

    // create the msg
    msg = MESSAGE_Alloc(node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        msg,
                        index,
                        TRACE_UMTS_LAYER3);

    memcpy((char*)MESSAGE_ReturnPacket(msg),
            pktBuff, index);

    // TODO: set the first byte of the gtp header
    unsigned char firstByte = UMTS_GTP_HEADER_FIRST_BYTE;
    UmtsAddGtpHeader(node, msg, firstByte,
                     UMTS_GTP_CREATE_PDP_CONTEXT_RESPONSE,
                     pdpInfo->dlTeId, NULL, 0);
    UmtsLayer3GsnSendGtpMsgOverBackbone(node, msg, pdpInfo->sgsnAddr);

    // update statistics
    gsnL3->ggsnStat.numCreatePDPContextRspSent ++;

    if ((DEBUG_GTP || DEBUG_SM) && pdpInfo->ueId == DEBUG_UE_ID)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_SM);
        printf("\n\tSend CREATE PDP CONTEXT RESPONSE to SGSN %d for "
               "NSAPI %d with DL TeId %d UL TEID %d\n",
               pdpInfo->sgsnId, pdpInfo->nsapi,
               pdpInfo->dlTeId,pdpInfo->ulTeId);
    }
}

// /**
// FUNCTION   :: UmtsLayer3GsnGtpSendDeletePDPContextRequest
// LAYER      :: Layer 3
// PURPOSE    :: Send a RAB ASSIGNMENT Release to one RNC for establishing
//               a GTP tunnelling
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + gsnL3     : UmtsLayer3Gsn*         : GSN data
// + naspi     : unsigned char          : NSAPI
// + ulTeId    : UInt32                 : UL TeId
// + dlTeId    : UInt32                 : DL TeId
// + gsnAddr   : Address                : Address of SGSN/GGSN
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3GsnGtpSendDeletePDPContextRequest(
          Node*node,
          UmtsLayer3Data* umtsL3,
          UmtsLayer3Gsn* gsnL3,
          unsigned char nsapi,
          UInt32 ulTeId,
          UInt32 dlTeId,
          Address gsnAddr)
{
    char pktBuff[UMTS_MAX_PACKET_BUFFER_SIZE];
    UmtsGtpDeletePDPContextRequest *pdpReqMsg;
    int index = 0;
    Message* msg;
    UInt32 teId;

    memset(pktBuff, 0, UMTS_MAX_PACKET_BUFFER_SIZE);
    pdpReqMsg = (UmtsGtpDeletePDPContextRequest *) &(pktBuff[0]);
    index += sizeof(UmtsGtpDeletePDPContextRequest);

    // fill the contents
    pdpReqMsg->nsapi = nsapi;

    // create the msg
    msg = MESSAGE_Alloc(node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        msg,
                        index,
                        TRACE_UMTS_LAYER3);

    memcpy((char*)MESSAGE_ReturnPacket(msg),
            pktBuff, index);

    // TODO: set the first byte of the gtp header
    unsigned char firstByte = UMTS_GTP_HEADER_FIRST_BYTE;
    if (gsnL3->isGGSN)
    {
        teId = dlTeId;
        // update statistics
        gsnL3->ggsnStat.numDeletePDPContextReqSent ++;
    }
    else
    {
        teId = ulTeId;
        // update statistics
        gsnL3->sgsnStat.numDeletePDPContextReqSent ++;
    }
    UmtsAddGtpHeader(node, msg, firstByte,
                     UMTS_GTP_DELETE_PDP_CONTEXT_REQUEST,
                     teId, NULL, 0);

    UmtsLayer3GsnSendGtpMsgOverBackbone(
        node, msg, gsnAddr);

    if ((DEBUG_GTP || DEBUG_SM))
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_SM);
        if (gsnL3->isGGSN)
        {
            printf("\n\tSend DELETE PDP CONTEXT REQUEST to SGSN for "
               "dL TeId %d\n",
               dlTeId);
        }
        else
        {
             printf("\n\tSend DELETE PDP CONTEXT REQUEST to GGSN for "
               "uL TeId %d\n",
               ulTeId);
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3GsnGtpSendDeletePDPContextResponse
// LAYER      :: Layer 3
// PURPOSE    :: Send a RAB ASSIGNMENT Release to one RNC for establishing
//               a GTP tunnelling
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + gsnL3     : UmtsLayer3Gsn*         : GSN data
// + naspi     : unsigned char          : NSAPI
// + ulTeId    : unsigned char          : UL TeId
// + dlTeId    : Unsigned char          : DL TeId
// + gtpCause  : UmtsGtpCauseType       : GTP cause type
// + gsnAddr   : Address                : Address of SGSN/GGSN
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3GsnGtpSendDeletePDPContextResponse(
          Node*node,
          UmtsLayer3Data* umtsL3,
          UmtsLayer3Gsn* gsnL3,
          unsigned char nsapi,
          unsigned char ulTeId,
          unsigned char dlTeId,
          UmtsGtpCauseType gtpCause,
          Address gsnAddr)
{
    char pktBuff[UMTS_MAX_PACKET_BUFFER_SIZE];
    UmtsGtpDeletePDPContextResponse *pdpRspMsg;
    int index = 0;
    Message* msg;
    unsigned char teId;

    memset(pktBuff, 0, UMTS_MAX_PACKET_BUFFER_SIZE);
    pdpRspMsg = (UmtsGtpDeletePDPContextResponse *) &(pktBuff[0]);
    index += sizeof(UmtsGtpDeletePDPContextResponse);

    // fill the contents
    pdpRspMsg->gtpCause = (unsigned char) gtpCause;

    // create the msg
    msg = MESSAGE_Alloc(node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        msg,
                        index,
                        TRACE_UMTS_LAYER3);

    memcpy((char*)MESSAGE_ReturnPacket(msg),
            pktBuff, index);

    // TODO: set the first byte of the gtp header
    unsigned char firstByte = UMTS_GTP_HEADER_FIRST_BYTE;
    if (gsnL3->isGGSN)
    {
        teId = dlTeId;

        // update statistics
        gsnL3->ggsnStat.numDeletePDPContextRspSent ++;
    }
    else
    {
        teId = ulTeId;

        // update statistics
        gsnL3->sgsnStat.numDeletePDPContextRspSent ++;
    }
    UmtsAddGtpHeader(node, msg, firstByte,
                     UMTS_GTP_DELETE_PDP_CONTEXT_RESPONSE,
                     teId, NULL, 0);
    UmtsLayer3GsnSendGtpMsgOverBackbone(node, msg, gsnAddr);

    if ((DEBUG_GTP || DEBUG_SM))
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_SM);
        if (gsnL3->isGGSN)
        {
            printf("\n\tSend DELETE PDP CONTEXT RESPONSE to SGSN for "
                   "NSAPI %d with DL TeId %d UL TEID %d\n",
                   nsapi, dlTeId, ulTeId);
        }
        else
        {
            printf("\n\tSend DELETE PDP CONTEXT RESPONSE to GGSN for "
                   "UL TEID %d\n",
                   ulTeId);
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3GsnGtpSendPduNotificationRequest
// LAYER      :: Layer 3
// PURPOSE    :: Send a PDU (Packet Data Unit) notification to SGSN to init
//               PDP context activation by network
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + gsnL3     : UmtsLayer3Gsn*         : GSN data
// + pdpInfo   : UmtsLayer3GgsnPdpInfo* : PDP info
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3GsnGtpSendPduNotificationRequest(
         Node* node,
         UmtsLayer3Data* umtsL3,
         UmtsLayer3Gsn* gsnL3,
         UmtsLayer3GgsnPdpInfo* pdpInfo)
{
    UmtsGtpPduNotificationRequest *pduReqMsg;
    unsigned char tfCls;
    unsigned char handlingPrio;
    Message* msg;
    int index = 0;
    char pktBuff[UMTS_MAX_PACKET_BUFFER_SIZE];

    memset(pktBuff, 0, UMTS_MAX_PACKET_BUFFER_SIZE);
    pduReqMsg = (UmtsGtpPduNotificationRequest *) &(pktBuff[0]);
    index += sizeof(UmtsGtpPduNotificationRequest);

    // fill the contents
    pduReqMsg->ulTeId = pdpInfo->ulTeId;
    pduReqMsg->ueId = pdpInfo->imsi.getId();

    // QoS info
    // set traffic class
    UmtsLayer3ConvertTrafficClassToOctet(
        pdpInfo->qosInfo->trafficClass,
        &tfCls,
        &handlingPrio);
    pduReqMsg->trafficClass = tfCls;
    pduReqMsg->trafficHandlingPrio = handlingPrio;

    // max bit rate
    UmtsLayer3ConvertMaxRateToOctet(pdpInfo->qosInfo->ulMaxBitRate,
                                    &pduReqMsg->ulMaxBitRate);
    UmtsLayer3ConvertMaxRateToOctet(pdpInfo->qosInfo->dlMaxBitRate,
                                    &pduReqMsg->dlMaxBitRate);
    UmtsLayer3ConvertMaxRateToOctet(pdpInfo->qosInfo->ulGuaranteedBitRate,
                                    &pduReqMsg->ulGuaranteedBitRate);
    UmtsLayer3ConvertMaxRateToOctet(pdpInfo->qosInfo->dlGuaranteedBitRate,
                                    &pduReqMsg->dlGuaranteedBitRate);

    // delay
    /*if (pdpInfo->qosInfo->trafficClass < UMTS_QOS_CLASS_INTERACTIVE_3)*/
    {
        // need set delay
        unsigned char delayVal;
        UmtsLayer3ConvertDelayToOctet(pdpInfo->qosInfo->transferDelay,
                                      &delayVal);

        pduReqMsg->transferDelay = delayVal;

    }

    pduReqMsg->qosIeLen = 13;

    // others
    pduReqMsg->pdpOrgType = 1; // IETF
    pduReqMsg->pdpAddrLen = 4; // IPv4, 16 Ipv6
    // pdpInfo->classifierInfo->srcAddr

    if (pdpInfo->srcDestType == UMTS_FLOW_NETWORK_ORIGINATED)
    {
        NodeAddress ipv4Addr =
            GetIPv4Address(pdpInfo->classifierInfo->dstAddr);
        memcpy(&pktBuff[index], (char*)&ipv4Addr, pduReqMsg->pdpAddrLen);
        index += pduReqMsg->pdpAddrLen;
    }

    // add extend maxBitRate

    // create the msg
    msg = MESSAGE_Alloc(node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        msg,
                        index,
                        TRACE_UMTS_LAYER3);

    memcpy((char*)MESSAGE_ReturnPacket(msg),
            pktBuff, index);

    // TODO: set the first byte of the gtp header
    unsigned char firstByte = UMTS_GTP_HEADER_FIRST_BYTE;
    UmtsAddGtpHeader(node, msg, firstByte,
                     UMTS_GTP_PDU_NOTIFICATION_REQUEST,
                     pdpInfo->ulTeId, NULL, 0);

    // this tlTeId should be used for any other purpose

    UmtsLayer3GsnSendGtpMsgOverBackbone(node, msg, pdpInfo->sgsnAddr);

    // update statistics
    gsnL3->ggsnStat.numPduNotificationReqSent ++;

    if ((DEBUG_GTP || DEBUG_SM) && pdpInfo->ueId == DEBUG_UE_ID)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_SM);
        printf("\n\tSend PDU Notification Request to SGSN %d with "
               "UL TeId %d\n",
               pdpInfo->sgsnId, pdpInfo->ulTeId);
    }
}

// /**
// FUNCTION   :: UmtsLayer3GsnGtpSendPduNotificationResponse
// LAYER      :: Layer 3
// PURPOSE    :: Send a PDU Notification Response to GGSN
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + gsnL3     : UmtsLayer3Gsn*         : GSN data
// + ueInfo    : UmtsSgsnUeDataInSgsn*  : Info of UE at the SGSN
// + flowInfo  : UmtsLayer3SgsnFlowInfo*: Info of the flow at SGSN
// + gtpCause  : UmtsGtpCauseType       : Result of the request
// + gsnAddr   : Address                : Address of SGSN/GGSN
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3GsnGtpSendPduNotificationResponse(
         Node*node,
         UmtsLayer3Data* umtsL3,
         UmtsLayer3Gsn* gsnL3,
         UmtsSgsnUeDataInSgsn* ueInfo,
         UmtsLayer3SgsnFlowInfo* flowInfo,
         UmtsGtpCauseType gtpCause,
         Address gsnAddr)
{
    UmtsGtpPduNotificationResponse* pduRepMsg;
    Message* msg;
    int msgSize;

    msgSize = sizeof(UmtsGtpPduNotificationResponse);

    // create the msg
    msg = MESSAGE_Alloc(node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        msg,
                        msgSize,
                        TRACE_UMTS_LAYER3);

    pduRepMsg = (UmtsGtpPduNotificationResponse*)MESSAGE_ReturnPacket(msg),
    memset((char*)pduRepMsg, 0, msgSize);

    // fill the contents
    pduRepMsg->gtpCause = (unsigned char)gtpCause;

    // TODO: set the first byte of the gtp header
    unsigned char firstByte = UMTS_GTP_HEADER_FIRST_BYTE;
    UmtsAddGtpHeader(node, msg, firstByte,
                     UMTS_GTP_PDU_NOTIFICATION_RESPONSE,
                     flowInfo->ulTeId, NULL, 0);

    // send out the message
    UmtsLayer3GsnSendGtpMsgOverBackbone(
        node, msg, gsnAddr);

    if ((DEBUG_GTP || DEBUG_SM) && flowInfo->ueId == DEBUG_UE_ID)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_SM);
        printf("\n\tSend PDU Notification to GGSN for UL TeId %d\n",
               flowInfo->ulTeId);
    }
}

// /**
// FUNCTION   :: UmtsLayer3GgsnGtpTunnelData
// LAYER      :: Layer 3
// PURPOSE    :: Tunnel a data packet to RNC via SGSN
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + gsnL3     : UmtsLayer3Gsn*         : GSN data
// + pdpInfo   : UmtsLayer3GgsnPdpInfo* : PDP Context info
// + msg       : Message*               : Data packet to be forwarde3d
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3GgsnGtpTunnelData(
         Node*node,
         UmtsLayer3Data* umtsL3,
         UmtsLayer3Gsn* gsnL3,
         UmtsLayer3GgsnPdpInfo* pdpInfo,
         Message* msg)
{
    if (DEBUG_GTP_DATA && DEBUG_UE_ID == pdpInfo->ueId)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_SM);
        printf ("tunnel a data packet to SGSN %d via GTP dlId %d.\n",
                pdpInfo->sgsnId, pdpInfo->dlTeId);
        UmtsPrintMessage(std::cout, msg);
    }

    unsigned char firstByte = UMTS_GTP_HEADER_FIRST_BYTE;
    UmtsAddGtpHeader(node,
                     msg,
                     firstByte,
                     UMTS_GTP_G_PDU,
                     pdpInfo->dlTeId,
                     NULL,
                     0);

    UmtsLayer3GsnSendGtpMsgOverBackbone(
        node,
        msg,
        pdpInfo->sgsnAddr,
        FALSE);

    // update statistics
    gsnL3->ggsnStat.numPsDataPacketToPlmn ++;
}

//////////////
/// VLR/HLR
//////////////
// /**
// FUNCTION   :: UmtsLayer3GsnPrintVlr
// LAYER      :: Layer 3
// PURPOSE    :: Print out the entries in VLR data base
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3GsnPrintVlr(Node *node, UmtsLayer3Data *umtsL3)
{
    UmtsLayer3Gsn *gsnL3 = (UmtsLayer3Gsn *)(umtsL3->dataPtr);
    UmtsVlrMap::iterator pos;
    UmtsVlrEntry *vlrEntry;
    char strBuff[MAX_STRING_LENGTH];
    UInt32 cellId;

    printf("UMTS Node%d (GSN)'s VLR data base has %u entries:\n",
           node->nodeId,
           (unsigned int)gsnL3->vlrMap->size());

    printf("UeID\t\tCell\tRNC\t\tIMSI\n");
    for (pos = gsnL3->vlrMap->begin(); pos != gsnL3->vlrMap->end(); pos ++)
    {
        vlrEntry = (UmtsVlrEntry *) pos->second;
        vlrEntry->imsi.print(strBuff);
        memcpy(&cellId, &(vlrEntry->rai.lac[0]), sizeof(cellId));
        printf("%d\t\t\t%d\t\t%d\t\t%s\n",
               vlrEntry->hashKey,
               cellId,
               vlrEntry->rncId,
               strBuff);
    }
}

// /**
// FUNCTION   :: UmtsLayer3GsnLookupVlr
// LAYER      :: Layer 3
// PURPOSE    :: Lookup VLR information for a given UE indicated by UE ID
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + ueId      : UInt32           : Identification of the UE
// RETURN     :: UmtsVlrEntry *   : Pointer to the VLR entry if found in VLR
//                                  NULL otherwise
// **/
static
UmtsVlrEntry *UmtsLayer3GsnLookupVlr(Node *node,
                                     UmtsLayer3Data *umtsL3,
                                     UInt32 ueId)
{
    UmtsLayer3Gsn *gsnL3 = (UmtsLayer3Gsn *)(umtsL3->dataPtr);
    UmtsVlrMap::iterator pos;

    pos = gsnL3->vlrMap->find(ueId);
    if (pos == gsnL3->vlrMap->end())
    {
        return NULL;
    }
    else
    {
        return (UmtsVlrEntry *)pos->second;
    }
}

// /**
// FUNCTION   :: UmtsLayer3GsnLookupVlr
// LAYER      :: Layer 3
// PURPOSE    :: Lookup VLR information for a given UE indicated by UE ID
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + ueId      : UInt32           : Identification of the UE
// + rncId     : UInt32 *         : For return ID of the RNC controls UE
// + cellId    : UInt32 *         : For return cell ID where UE is located
// RETURN     :: BOOL : TRUE if found in VLR, FALSE otherwise
// **/
static
BOOL UmtsLayer3GsnLookupVlr(Node *node,
                            UmtsLayer3Data *umtsL3,
                            UInt32 ueId,
                            UInt32 *rncId,
                            UInt32 *cellId)
{
    UmtsVlrEntry *vlrEntry = NULL;

    vlrEntry = UmtsLayer3GsnLookupVlr(node, umtsL3, ueId);
    if (vlrEntry != NULL)
    {
        if (rncId != NULL)
        {
            *rncId = vlrEntry->rncId;
        }

        if (cellId != NULL)
        {
            memcpy(cellId, &(vlrEntry->rai.lac[0]), sizeof(UInt32));
        }

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

// /**
// FUNCTION   :: UmtsLayer3GsnQueryHlr
// LAYER      :: Layer 3
// PURPOSE    :: Query info of UE from HLR
// PARAMETERS ::
// + node      : Node *           : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + imsi      : CellularIMSI     : IMSI of the UE
// RETURN     :: void             : NULL
// **/
static
void UmtsLayer3GsnQueryHlr(Node *node,
                           UmtsLayer3Data *umtsL3,
                           CellularIMSI imsi)
{
    UmtsLayer3Gsn *gsnL3 = (UmtsLayer3Gsn *)(umtsL3->dataPtr);
    UmtsHlrRequest *hlrReq;
    Message *msg = NULL;

    msg = MESSAGE_Alloc(node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        msg,
                        sizeof(UmtsHlrRequest),
                        TRACE_UMTS_LAYER3);
    hlrReq = (UmtsHlrRequest *) MESSAGE_ReturnPacket(msg);
    ERROR_Assert(hlrReq != NULL, "UMTS: Memory error!");

    memset((char*)hlrReq, 0, sizeof(UmtsHlrRequest));

    hlrReq->command = UMTS_HLR_QUERY;
    imsi.getCompactIMSI((char*)&(hlrReq->imsi[0]));

    // TODO: What if IMSI's MCC and MNC are different from mine? This means
    // the HLR of the UE is in another PLMN
    UmtsLayer3SgsnSendMsgToHlr(node, umtsL3, msg, gsnL3->hlrAddr);
}

// /**
// FUNCTION   :: UmtsLayer3GsnUpdateHlr
// LAYER      :: Layer 3
// PURPOSE    :: Update information in HLR for a UE
// PARAMETERS ::
// + node      : Node *           : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + vlrEntry  : UmtsVlrEntry *   : Pointer to the entry in VLR
// RETURN     :: void             : NULL
// **/
static
void UmtsLayer3GsnUpdateHlr(Node *node,
                            UmtsLayer3Data *umtsL3,
                            UmtsVlrEntry *vlrEntry)
{
    UmtsLayer3Gsn *gsnL3 = (UmtsLayer3Gsn *)(umtsL3->dataPtr);
    UmtsHlrRequest hlrReq;
    UmtsHlrLocationInfo locInfo;
    Message *msg = NULL;

    memset((char*)&hlrReq, 0, sizeof(UmtsHlrRequest));
    memset((char*)&locInfo, 0, sizeof(UmtsHlrLocationInfo));

    hlrReq.command = UMTS_HLR_UPDATE;
    vlrEntry->imsi.getCompactIMSI((char*)&(hlrReq.imsi[0]));
    locInfo.sgsnId = gsnL3->gsnId;

    msg = MESSAGE_Alloc(node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        msg,
                        sizeof(UmtsHlrRequest) +
                            sizeof(UmtsHlrLocationInfo),
                        TRACE_UMTS_LAYER3);

    char* packet = MESSAGE_ReturnPacket(msg);
    memcpy(packet, &hlrReq, sizeof(hlrReq));
    memcpy(packet + sizeof(hlrReq), &locInfo, sizeof(locInfo));

    // TODO: What if IMSI's MCC and MNC are different from mine? This means
    // the HLR of the UE is in another PLMN
    UmtsLayer3SgsnSendMsgToHlr(node, umtsL3, msg, gsnL3->hlrAddr);
}

// /**
// FUNCTION   :: UmtsLayer3GsnUpdateHlrCellInfo
// LAYER      :: Layer 3
// PURPOSE    :: Update information in HLR for a UE
// PARAMETERS ::
// + node      : Node *           : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + cellId    : UInt32           : cellId of the nodeB
// + nodeId    : NodeId           : Node id of Nodeb
// + rncId     : NodeId           : Node Id of RNC
// RETURN     :: void             : NULL
// **/
static
void UmtsLayer3GsnUpdateHlrCellInfo(Node *node,
                                    UmtsLayer3Data *umtsL3,
                                    UInt32 cellId,
                                    NodeId nodebId,
                                    NodeId rncId)
{
    UmtsLayer3Gsn *gsnL3 = (UmtsLayer3Gsn *)(umtsL3->dataPtr);
    Message *msg = NULL;

    std::string msgBuf;
    msgBuf += (char) UMTS_HLR_UPDATE_CELL;
    msgBuf.append((char*)&cellId, sizeof(cellId));
    msgBuf.append((char*)&nodebId, sizeof(nodebId));
    msgBuf.append((char*)&rncId, sizeof(rncId));
    msgBuf.append((char*)&(node->nodeId), sizeof(node->nodeId));

    msg = MESSAGE_Alloc(node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        msg,
                        (int)msgBuf.size(),
                        TRACE_UMTS_LAYER3);
    memcpy(MESSAGE_ReturnPacket(msg),
           msgBuf.data(),
           msgBuf.size());

    UmtsLayer3SgsnSendMsgToHlr(node,
                               umtsL3,
                               msg,
                               gsnL3->hlrAddr,
                               UMTS_HLR_COMMAND_CELL);
}

// /**
// FUNCTION   :: UmtsLayer3GsnQueryHlrCellInfo
// LAYER      :: Layer 3
// PURPOSE    :: Update information in HLR for a UE
// PARAMETERS ::
// + node      : Node *           : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + cellId    : UInt32           : cellId of the nodeB
// + rncId     : NodeId           : Node Id of RNC
// RETURN     :: void             : NULL
// **/
static
void UmtsLayer3GsnQueryHlrCellInfo(Node *node,
                                   UmtsLayer3Data *umtsL3,
                                   UInt32 cellId,
                                   NodeId rncId)
{
    UmtsLayer3Gsn *gsnL3 = (UmtsLayer3Gsn *)(umtsL3->dataPtr);
    Message *msg = NULL;

    std::string msgBuf;
    msgBuf += (char) UMTS_HLR_QUERY_CELL;
    msgBuf.append((char*)&cellId, sizeof(cellId));
    msgBuf.append((char*)&rncId, sizeof(rncId));

    msg = MESSAGE_Alloc(node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        msg,
                        (int)msgBuf.size(),
                        TRACE_UMTS_LAYER3);
    memcpy(MESSAGE_ReturnPacket(msg),
           msgBuf.data(),
           msgBuf.size());

    UmtsLayer3SgsnSendMsgToHlr(node,
                               umtsL3,
                               msg,
                               gsnL3->hlrAddr,
                               UMTS_HLR_COMMAND_CELL);
}

// /**
// FUNCTION   :: UmtsLayer3GsnSendCellLookupReply
// LAYER      :: Layer 3
// PURPOSE    :: Update information in HLR for a UE
// PARAMETERS ::
// + node      : Node *           : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + cellId    : UInt32           : cellId of the nodeB
// + nodeId    : NodeId           : Node id of Nodeb
// + rncId     : NodeId           : Node Id of RNC
// + queryRncId : NodeId          : querying RND's Node  Id
// RETURN     :: void             : NULL
// **/
static
void UmtsLayer3GsnSendCellLookupReply(
    Node *node,
    UmtsLayer3Data *umtsL3,
    UInt32 cellId,
    NodeId nodebId,
    NodeId rncId,
    NodeId queryRncId)
{
    std::string msgBuf;
    msgBuf.append((char*)&cellId, sizeof(cellId));
    msgBuf.append((char*)&nodebId, sizeof(nodebId));
    msgBuf.append((char*)&rncId, sizeof(rncId));

    Message* msg =
        MESSAGE_Alloc(node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node, msg, (int)msgBuf.size(), TRACE_UMTS_LAYER3);
    memcpy(MESSAGE_ReturnPacket(msg),
           msgBuf.data(),
           msgBuf.size());

    UmtsLayer3SgsnSendRanapMsgToRnc(
       node,
       umtsL3,
       msg,
       0,
       UMTS_RANAP_MSG_TYPE_CELL_LOOKUP_REPLY,
       queryRncId);
}

// /**
// FUNCTION   :: UmtsLayer3GsnHandleLocUpReq
// LAYER      :: Layer 3
// PURPOSE    :: Handle a Mobility Management message from a UE .
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + transactId     : unsigned char          : Transaction ID
// + msg            : char*      : Pointer to the NAS message contents
// + msgSize        : int                    : Message size
// + ueId           : NodeId                 : UE ID
// + rncId          : NodeId                 : RNC ID
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3GsnHandleLocUpReq(
    Node*               node,
    UmtsLayer3Data*     umtsL3,
    unsigned char       transactId,
    char*               msg,
    int                 msgSize,
    NodeId              ueId,
    NodeId              rncId)
{
    UmtsLayer3Gsn *gsnL3 = (UmtsLayer3Gsn *)(umtsL3->dataPtr);
    UmtsVlrMap::iterator pos;
    UmtsVlrEntry *vlrEntry;
    UmtsLayer3RoutingUpdateRequest reqMsg;
    int index = 0;
    BOOL isNewEntry = FALSE;

    if (DEBUG_MM)
    {
        char timeStr[20];
        TIME_PrintClockInSecond(node->getNodeTime(), timeStr);
        printf("%s, SGSN: %u receives a Location Updating Request"
            " message from UE: %u, rncId %d\n",
            timeStr, node->nodeId, ueId, rncId);
    }

    // update statistics
    gsnL3->sgsnStat.numRoutingUpdateRcvd ++;

    memcpy(&reqMsg, msg, sizeof(reqMsg));
    index += sizeof(UmtsLayer3RoutingUpdateRequest);

    // update the VLR data base
    pos = gsnL3->vlrMap->find(ueId);
    if (pos == gsnL3->vlrMap->end())
    {
        // not in VLR data, add it
        isNewEntry = TRUE;
        vlrEntry = (UmtsVlrEntry *) MEM_malloc(sizeof(UmtsVlrEntry));
        memset((char*)vlrEntry, 0, sizeof(UmtsVlrEntry));
        vlrEntry->hashKey = ueId;

        gsnL3->vlrMap->insert(
            UmtsVlrMap::value_type(vlrEntry->hashKey, vlrEntry));

        // update VLR statics
        gsnL3->sgsnStat.numVlrEntryAdded ++;

        // dynamic statistics
        if (node->guiOption)
        {
            GUI_SendIntegerData(
                node->nodeId,
                gsnL3->sgsnStat.numRegedUeGuiId,
                gsnL3->sgsnStat.numVlrEntryAdded -
                    gsnL3->sgsnStat.numVlrEntryRemoved,
                node->getNodeTime());
        }
    }
    else
    {
        vlrEntry = pos->second;
    }

    // update the routing area ID
    memcpy((char*)&(vlrEntry->rai),
           (char*)&(reqMsg.oldRai),
           sizeof(UmtsRoutingAreaId));
    vlrEntry->imsi.setCompactIMSI(&(msg[index]));
    vlrEntry->rncId = rncId;

    // if new entry, report to HLR
    if (isNewEntry)
    {
        // if IMSI indicates its in same PLMN as me, then report to my HLR
        // Otherwise, should report to the HLR of the UE's home PLMN
        UmtsLayer3GsnUpdateHlr(node, umtsL3, vlrEntry);

        // update HLR statistics
        gsnL3->sgsnStat.numHlrUpdate ++;
    }
    else
    {
        unsigned char upInterval;
        unsigned char upUnit = 0; // 2 SECOND per UNIT
        upInterval = (upUnit << 5) | UMTS_LAYER3_DEFAULT_RA_UPDATE_INTERVAL;
        // Send Location update accpet to UE
        UmtsLayer3SgsnSendLocationUpdateAccept(
            node, umtsL3, ueId, 1, upInterval);
    }

    if (DEBUG_MM && 0)
    {
        UmtsLayer3GsnPrintVlr(node, umtsL3);
    }

    std::list<UmtsSgsnUeDataInSgsn*>::iterator ueNasIt;
    for (ueNasIt = gsnL3->ueInfoList->begin();
         ueNasIt != gsnL3->ueInfoList->end();
         ++ueNasIt)
    {
        if ((*ueNasIt)->imsi == vlrEntry->imsi)
        {
            break;
        }
    }

    if (ueNasIt == gsnL3->ueInfoList->end())
    {
        UmtsSgsnUeDataInSgsn* ueNas;
        ueNas= UmtsLayer3GsnAddUeInfo(
                node,
                umtsL3,
                gsnL3,
                ueId,
                rncId,
                vlrEntry->imsi);
    }
    else
    {
        //If the timer T3255 is on, stop it
        UmtsMmNwData* ueMm = (*ueNasIt)->mmData;
        if (ueMm->timerT3255)
        {
            MESSAGE_CancelSelfMsg(node, ueMm->timerT3255);
            ueMm->timerT3255 = NULL;
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3GsnHandleCmServReq
// LAYER      :: Layer 3
// PURPOSE    :: Handle a CM SERVICE REQUEST from a UE .
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + ti             : unsigned char          : transaction Id
// + msg            : char* : Pointer to the NAS message contents
// + msgSize        : int                    : Message size
// + ueId           : NodeId                 : UE ID
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3GsnHandleCmServReq(
    Node*               node,
    UmtsLayer3Data*     umtsL3,
    unsigned char       ti,
    char*               msg,
    int                 msgSize,
    NodeId              ueId)
{
    UmtsLayer3Gsn *gsnL3 = (UmtsLayer3Gsn *)(umtsL3->dataPtr);

    UmtsLayer3PD pd;
    memcpy(&pd, msg, sizeof(pd));

    if (DEBUG_MM)
    {
        char timeStr[20];
        TIME_PrintClockInSecond(node->getNodeTime(), timeStr);
        printf("%s, SGSN: %u receives a CM Service Request"
            " message from UE: %u, PD: %u, TI: %u \n",
            timeStr, node->nodeId, ueId, pd, ti);
    }

    UmtsVlrMap::iterator vlrIter = gsnL3->vlrMap->find(ueId);
    std::list<UmtsSgsnUeDataInSgsn*>::iterator ueNasIter;
    ueNasIter = find_if(gsnL3->ueInfoList->begin(),
                        gsnL3->ueInfoList->end(),
                        UmtsFindItemByUeId<UmtsSgsnUeDataInSgsn>(ueId));

    if (vlrIter != gsnL3->vlrMap->end()
        && ueNasIter != gsnL3->ueInfoList->end())
    {
        UmtsMmNwData* ueMm = (*ueNasIter)->mmData;
        if (ueMm->mainState == UMTS_MM_NW_IDLE ||
                ueMm->mainState == UMTS_MM_NW_WAIT_FOR_RR_CONNECTION)
        {
            ueMm->mainState =
                UMTS_MM_NW_WAIT_FOR_MOBILE_ORIGINATED_MM_CONNECTION;
        }
        UmtsLayer3SgsnSendCmServAcpt(node, umtsL3, *ueNasIter, pd, ti);
    }
}

// /**
// FUNCTION   :: UmtsLayer3GsnHandlePagingResp
// LAYER      :: Layer 3
// PURPOSE    :: Handle a PAGING RESPONSE from a UE .
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + ti             : unsigned char          : transaction Id
// + msg            : char*   : Pointer to the NAS message contents
// + msgSize        : int                    : Message size
// + ueId           : NodeId                 : UE ID
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3GsnHandlePagingResp(
    Node*               node,
    UmtsLayer3Data*     umtsL3,
    unsigned char       ti,
    char*               msg,
    int                 msgSize,
    NodeId              ueId)
{
    UmtsLayer3Gsn *gsnL3 = (UmtsLayer3Gsn *)(umtsL3->dataPtr);

    if (DEBUG_MM)
    {
        char timeStr[20];
        TIME_PrintClockInSecond(node->getNodeTime(), timeStr);
        printf("%s, SGSN: %u receives a PAGING RESPONSE"
            " message from UE: %u\n ",
            timeStr, node->nodeId, ueId);
    }

    std::list<UmtsSgsnUeDataInSgsn*>::iterator ueNasIter;
    ueNasIter = find_if(gsnL3->ueInfoList->begin(),
                        gsnL3->ueInfoList->end(),
                        UmtsFindItemByUeId<UmtsSgsnUeDataInSgsn>(ueId));

    if (ueNasIter != gsnL3->ueInfoList->end())
    {
        UmtsMmNwData* ueMm = (*ueNasIter)->mmData;
        if (ueMm->mainState == UMTS_MM_NW_WAIT_FOR_RR_CONNECTION)
        {
            UmtsLayer3GsnMmConnEstInd(node, umtsL3, *ueNasIter);
            ueMm->mainState = UMTS_MM_NW_CONNECTION_ACTIVE;
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3GsnHandlePacketFromHlr
// LAYER      :: Layer 3
// PURPOSE    :: Handle a Mobility Management message from a UE .
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + pktBuff   : char *           : Pointer to the packet
// RETURN     :: void             : NULL
// **/
static
void UmtsLayer3GsnHandlePacketFromHlr(Node *node,
                                     UmtsLayer3Data *umtsL3,
                                     char *pktBuff)
{
    UmtsLayer3Gsn *gsnL3 = (UmtsLayer3Gsn *) (umtsL3->dataPtr);
    UmtsHlrReply replyPkt;
    CellularIMSI tmpImsi;
    int index = 0;
    UInt32 ueId;

    memcpy(&replyPkt, pktBuff, sizeof(replyPkt));
    index += sizeof(replyPkt);
    tmpImsi.setCompactIMSI((char*)&(replyPkt.imsi[0]));
    ueId = tmpImsi.getId();

    if (DEBUG_PACKET)
    {
        printf("UMTS Node%d(GSN) receives a reply from HLR for UE %d:",
               node->nodeId, ueId);
    }

    switch (replyPkt.command)
    {
        case UMTS_HLR_UPDATE_REPLY:
        {
            if (DEBUG_PACKET)
            {
                printf("UPDATE-REPLY packet with result %d\n",
                       replyPkt.result);
            }

            if (replyPkt.result == UMTS_HLR_COMMAND_SUCC)
            {
                // location update request was successful
                unsigned char upInterval;
                unsigned char upUnit = 0; // 2 SECOND per UNIT
                upInterval = (upUnit << 5) |
                    UMTS_LAYER3_DEFAULT_RA_UPDATE_INTERVAL;
                // Send Location update accpet to UE
                UmtsLayer3SgsnSendLocationUpdateAccept(
                    node, umtsL3, ueId, 1, upInterval);

                //Release CS signal connection is handled by MS T3240
#if 0
                //Set a timer to release RRC connection
                std::list<UmtsSgsnUeDataInSgsn*>::iterator ueNasIter
                    = std::find_if(gsnL3->ueInfoList->begin(),
                                   gsnL3->ueInfoList->end(),
                                   UmtsFindItemByUeId
                                        <UmtsSgsnUeDataInSgsn>(ueId));
                UmtsMmNwData* ueMm = (*ueNasIter)->mmData;
                ueMm->timerT3255 = UmtsLayer3SetTimer(
                                        node,
                                        umtsL3,
                                        UMTS_LAYER3_TIMER_T3255,
                                        UMTS_LAYER3_TIMER_T3255_VAL,
                                        NULL,
                                        &ueId,
                                        sizeof(ueId));
#endif
            }
            else
            {
                // location update request failed
            }

            break;
        }

        case UMTS_HLR_QUERY_REPLY:
        {
            if (DEBUG_PACKET)
            {
                printf("QUERY-REPLY packet with result %d\n",
                       replyPkt.result);
            }

            if (replyPkt.result == UMTS_HLR_COMMAND_SUCC)
            {
                // successful, the pkt contains location info of the UE
                UmtsHlrLocationInfo locInfo;
                memcpy(&locInfo, &pktBuff[index], sizeof(locInfo));

                // update statistics
                if (gsnL3->isGGSN)
                {
                    gsnL3->ggsnStat.numRoutingQueryReplyRcvd ++;
                }

                if (DEBUG_PACKET)
                {
                    printf("    UE %d is under area of SGSN %d\n",
                           ueId, locInfo.sgsnId);
                }
                if (gsnL3->isGGSN)
                {
                    UmtsLayer3GgsnCheckPdpPendingOnRouting(
                        node,
                        umtsL3,
                        tmpImsi,
                        &locInfo);

                    UmtsLayer3GmscCheckCallPendingOnRouting(
                        node,
                        umtsL3,
                        ueId,
                        &locInfo);
                }
            }
            else
            {
                // HLR didn't have location info for the UE
                // update statistics
                if (gsnL3->isGGSN)
                {
                    gsnL3->ggsnStat.numRoutingQueryFailRcvd ++;
                    UmtsLayer3GgsnCheckPdpPendingOnRouting(
                        node,
                        umtsL3,
                        tmpImsi,
                        NULL);

                    UmtsLayer3GmscCheckCallPendingOnRouting(
                        node,
                        umtsL3,
                        ueId,
                        NULL);
                }
            }
            break;
        }

        case UMTS_HLR_REMOVE:
        {
            if (DEBUG_PACKET)
            {
                printf("REMOVE packet to remove UE %d from VLR\n",
                       ueId);
            }

            // another SGSN has updated HLR, so UE moved, HLR instructed
            // me to remove the UE from VLR
            gsnL3->vlrMap->erase(ueId);

            UmtsLayer3GsnRemoveUeInfo(node, umtsL3, gsnL3, ueId);

            // update statistics
            gsnL3->sgsnStat.numVlrEntryRemoved ++;

            // dynamic statistics
            if (node->guiOption)
            {
                GUI_SendIntegerData(
                    node->nodeId,
                    gsnL3->sgsnStat.numRegedUeGuiId,
                    gsnL3->sgsnStat.numVlrEntryAdded -
                        gsnL3->sgsnStat.numVlrEntryRemoved,
                    node->getNodeTime());
            }
            break;
        }
        case UMTS_HLR_REMOVE_REPLY:
        {
            if (DEBUG_PACKET)
            {
                printf("REMOVE-REPLY packet with result %d\n",
                       replyPkt.result);
            }

            if (replyPkt.result == UMTS_HLR_COMMAND_SUCC)
            {
                // remove request was successful
            }
            else
            {
                // remove request failed
            }

            break;
        }

        default:
        {
            // unsupported reply type
            char errorStr[MAX_STRING_LENGTH];
            sprintf(errorStr,
                    "UMTS Node%d (GSN) receives a packet from HLR with "
                    "unknown type %d\n",
                    node->nodeId,
                    replyPkt.command);
            ERROR_ReportWarning(errorStr);
            break;
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3GsnHandleHlrCellReply
// LAYER      :: Layer 3
// PURPOSE    :: Handle a Mobility Management message from a UE.
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + packet    : char*            : Pointer to the packet
// RETURN     :: void             : NULL
// **/
static
void UmtsLayer3GsnHandleHlrCellReply(
        Node *node,
        UmtsLayer3Data *umtsL3,
        const char *packet)
{
    UmtsLayer3Gsn *gsnL3 = (UmtsLayer3Gsn *) (umtsL3->dataPtr);
    int index = 0;
    UmtsHlrCommand cmdType = (UmtsHlrCommand) packet[index ++];
    if (cmdType == UMTS_HLR_QUERY_CELL_REPLY)
    {
        UInt32 cellId;
        NodeId queryRncId;
        NodeId nodebId;
        NodeId rncId;
        NodeId sgsnId;
        Address sgsnAddr;
        memcpy(&cellId, &packet[index], sizeof(cellId));
        index += sizeof(cellId);
        memcpy(&queryRncId, &packet[index], sizeof(queryRncId));
        index += sizeof(queryRncId);
        memcpy(&nodebId, &packet[index], sizeof(nodebId));
        index += sizeof(nodebId);
        memcpy(&rncId, &packet[index], sizeof(rncId));
        index += sizeof(rncId);
        memcpy(&sgsnId, &packet[index], sizeof(sgsnId));
        index += sizeof(sgsnId);
        memcpy(&sgsnAddr, &packet[index], sizeof(sgsnAddr));
        index += sizeof(sgsnAddr);

        UmtsRncCacheMap::iterator itRnc
            = gsnL3->rncCacheMap->find(rncId);
        if (gsnL3->rncCacheMap->end() == itRnc)
        {
            gsnL3->rncCacheMap->insert(
                        std::make_pair(
                             rncId,
                             new UmtsRncCacheInfo(
                                 sgsnId,
                                 sgsnAddr)));
        }

        UmtsCellCacheMap::iterator it
            = gsnL3->cellCacheMap->find(cellId);
        if (gsnL3->cellCacheMap->end() == it)
        {
            gsnL3->cellCacheMap->insert(
                        std::make_pair(
                             cellId,
                             new UmtsCellCacheInfo(
                                 nodebId,
                                 rncId)));
        }
        else
        {
            it->second->nodebId = nodebId;
            it->second->rncId = rncId;
        }
        UmtsLayer3GsnSendCellLookupReply(
            node,
            umtsL3,
            cellId,
            nodebId,
            rncId,
            queryRncId);
    }
    else if (cmdType == UMTS_HLR_UPDATE_CELL_REPLY)
    {
        UInt32 cellId;
        memcpy(&cellId, &packet[index], sizeof(cellId));
        UmtsCellCacheMap::iterator it
            = gsnL3->cellCacheMap->find(cellId);
        it->second->hlrUpdated = TRUE;
    }
    else if (cmdType == UMTS_HLR_RNC_UPDATE)
    {
        NodeId rncId;
        NodeId sgsnId;
        Address sgsnAddr;
        memcpy(&rncId, &packet[index], sizeof(rncId));
        index += sizeof(rncId);
        memcpy(&sgsnId, &packet[index], sizeof(sgsnId));
        index += sizeof(sgsnId);
        memcpy(&sgsnAddr, &packet[index], sizeof(sgsnAddr));
        index += sizeof(sgsnAddr);
        UmtsRncCacheMap::iterator itRnc
            = gsnL3->rncCacheMap->find(rncId);
        if (itRnc == gsnL3->rncCacheMap->end())
        {
            gsnL3->rncCacheMap->insert(
                std::make_pair(rncId,
                               new UmtsRncCacheInfo(
                                   sgsnId,
                                   sgsnAddr)));
        }
    }
    else
    {
        ERROR_ReportError("Rcvd invalid HLR cell command type");
    }
}

// FUNCTION   :: UmtsLayer3GsnReqMmConnEst
// LAYER      :: Layer3 MM
// PURPOSE    :: Request to establish MM connection by CM entities
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data*   : Pointer to L3 data
// + ueNas            : UmtsSgsnUeDataInSgsn* : UE data at SGSN
// + pd               : UmtsLayer3PD      : Protocol discrimator
// + ti               : unsigned char     : Transaction ID
// RETURN     :: BOOL : TRUE if the MM connection is active
// **/
static
BOOL UmtsLayer3GsnReqMmConnEst(
     Node* node,
     UmtsLayer3Data* umtsL3,
     UmtsSgsnUeDataInSgsn* ueNas,
     UmtsLayer3PD pd,
     unsigned char ti)
{
    BOOL retVal = FALSE;
    UmtsMmNwData* ueMm = ueNas->mmData;
    if (ueMm->mainState == UMTS_MM_NW_IDLE)
    {
        UmtsLayer3SgsnSendPaging(node,
                                 umtsL3,
                                 ueNas->ueId,
                                 UMTS_LAYER3_CN_DOMAIN_CS,
                                 UMTS_PAGING_CAUSE_CONVERSATION_CALL);
        ueMm->mainState = UMTS_MM_NW_WAIT_FOR_RR_CONNECTION;
    }
    else if (ueMm->mainState == UMTS_MM_NW_CONNECTION_ACTIVE
             || ueMm->mainState ==
                UMTS_MM_NW_WAIT_FOR_MOBILE_ORIGINATED_MM_CONNECTION
             || ueMm->mainState ==
                UMTS_MM_NW_WAIT_FOR_NETWORK_ORIGINATED_MM_CONNECTION)
    {
        retVal = TRUE;
        UmtsLayer3MmConn cmpMmConn = UmtsLayer3MmConn(pd, ti);
        if (ueMm->actMmConns->end() ==
                find_if(ueMm->actMmConns->begin(),
                        ueMm->actMmConns->end(),
                        bind2nd(UmtsPointedItemEqual<UmtsLayer3MmConn>(),
                                &cmpMmConn)))
        {
            ueMm->actMmConns->push_back(new UmtsLayer3MmConn(cmpMmConn));
        }

        ueMm->mainState = UMTS_MM_NW_CONNECTION_ACTIVE;
    }
    return retVal;
}

// FUNCTION   :: UmtsLayer3GsnReleaseMmConn
// LAYER      :: Layer3 MM
// PURPOSE    :: Request to release MM connection by CM entities
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data*   : Pointer to L3 data
// + ueNas            : UmtsSgsnUeDataInSgsn* : UE data at SGSN
// + pd               : UmtsLayer3PD      : Protocol discrimator
// + ti               : unsigned char     : Transaction ID
// RETURN     :: NULL
// **/
static
void UmtsLayer3GsnReleaseMmConn(
     Node* node,
     UmtsLayer3Data* umtsL3,
     UmtsSgsnUeDataInSgsn* ueNas,
     UmtsLayer3PD pd,
     unsigned char ti)
{
    UmtsMmNwData* ueMm = ueNas->mmData;
    UmtsLayer3MmConn cmpMmConn = UmtsLayer3MmConn(pd, ti);

    UmtsMmConnCont::iterator mmConnIter =
            find_if(ueMm->actMmConns->begin(),
                    ueMm->actMmConns->end(),
                    bind2nd(UmtsPointedItemEqual<UmtsLayer3MmConn>(),
                            &cmpMmConn));
    if (ueMm->actMmConns->end() != mmConnIter)
    {
        delete *mmConnIter;
        ueMm->actMmConns->erase(mmConnIter);
        if (ueMm->actMmConns->empty())
        {
            ueMm->mainState = UMTS_MM_NW_IDLE;
            //Release CS signalling connection is handled by UE
            //side
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3GsnHandleMmMsg
// LAYER      :: Layer 3
// PURPOSE    :: Handle a Mobility Management message from a UE .
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msgType        : UmtsMmMessageType      : Message type
// + transactId     : unsigned char          : Transaction ID
// + msg            : char*   : Pointer to the NAS message contents
// + msgSize        : int                    : Message size
// + ueId           : NodeId                 : UE ID
// + rncId          : NodeId                 : RNC ID
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3GsnHandleMmMsg(
    Node*               node,
    UmtsLayer3Data*     umtsL3,
    UmtsMmMessageType   msgType,
    unsigned char       transactId,
    char*               msg,
    int                 msgSize,
    NodeId              ueId,
    NodeId              rncId)
{
    switch (msgType)
    {
        case UMTS_MM_MSG_TYPE_LOCATION_UPDATING_REQUEST:
        {
            UmtsLayer3GsnHandleLocUpReq(
                node,
                umtsL3,
                transactId,
                msg,
                msgSize,
                ueId,
                rncId);
            break;
        }
        case UMTS_MM_MSG_TYPE_CM_SERVICE_REQUEST:
        {
            UmtsLayer3GsnHandleCmServReq(
                node,
                umtsL3,
                transactId,
                msg,
                msgSize,
                ueId);
            break;
        }
        case UMTS_MM_MSG_TYPE_PAGING_RESPONSE:
        {
            UmtsLayer3GsnHandlePagingResp(
                node,
                umtsL3,
                transactId,
                msg,
                msgSize,
                ueId);
            break;
        }
        default:
        {
            char errStr[MAX_STRING_LENGTH];
            sprintf(errStr,
                    "UMTS Layer3: SGSN: %d receivers a MM "
                    "signalling msg with unknown type: %d",
                    node->nodeId,
                    msgType);
            ERROR_ReportWarning(errStr);
        }
    }
}

//////////////////
//GMM
///////////////////

// /**
// FUNCTION   :: Umtslayer3GsnGMmSendAttachAccept
// LAYER      :: Layer3 GMM
// PURPOSE    :: Send a Attach request
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data*   : Pointer to L3 data
// + gsnL3            : UmtsLayer3Gsn     : GSN layer3 data
// + attachResult     : UmtsAttachResult  : attach result
// + ueId             : NodeId            : UE Id
// RETURN     :: void : NULL
// **/
static
void Umtslayer3GsnGMmSendAttachAccept(Node* node,
                               UmtsLayer3Data* umtsL3,
                               UmtsLayer3Gsn *gsnL3,
                               UInt32 tmsi,
                               UmtsAttachResult attachResult,
                               NodeId ueId)
{
    char pktBuff[UMTS_MAX_PACKET_BUFFER_SIZE];
    UmtsLayer3AttachAccept *attachAcptMsg;
    int index = 0;

    memset(pktBuff, 0, UMTS_MAX_PACKET_BUFFER_SIZE);
    attachAcptMsg = (UmtsLayer3AttachAccept *) &(pktBuff[0]);
    index += sizeof(UmtsLayer3AttachAccept);

    // fill the contents
    attachAcptMsg->attachResult = (unsigned char)attachResult;

    // TODO
    // include the TMSI/P-TMSI in the packet
    memcpy((unsigned char*)&(pktBuff[index]),
           (unsigned char*)&tmsi,
           sizeof(UInt32));
    index += sizeof(UInt32);

    UmtsLayer3GsnSendNasMsg(
        node,
        umtsL3,
        pktBuff,
        index,
        (char)UMTS_LAYER3_PD_GMM,
        (char)UMTS_GMM_MSG_TYPE_ATTACH_ACCEPT,
        0,
        ueId);
}

// /**
// FUNCTION   :: UmtsLayer3GsnHandleAttachReq
// LAYER      :: Layer 3
// PURPOSE    :: Handle a Mobility Management message from a UE .
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + transactId     : unsinged char          : Transaction ID
// + msg            : char*   : Pointer to the NAS message contents
// + msgSize        : int                    : Message size
// + ueId           : NodeId                 : UE ID
// + rncId          : NodeId                 : RNC ID
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3GsnHandleAttachReq(
    Node*               node,
    UmtsLayer3Data*     umtsL3,
    unsigned char       transactId,
    char*               msg,
    int                 msgSize,
    NodeId              ueId,
    NodeId              rncId)
{
    UmtsLayer3Gsn *gsnL3 = (UmtsLayer3Gsn *)(umtsL3->dataPtr);
    UmtsLayer3AttachRequest reqMsg;
    int index = 0;
    //BOOL isNewUe = FALSE;
    CellularIMSI imsi;
    UmtsSgsnUeDataInSgsn*  newUeInfo;
    UmtsAttachType attachType;

    if (DEBUG_GMM && DEBUG_UE_ID == ueId)
    {
        char timeStr[20];
        TIME_PrintClockInSecond(node->getNodeTime(), timeStr);
        printf("%s, SGSN: %u receives a Attach Request"
            " message from UE: %u, rncId %d\n",
            timeStr, node->nodeId, ueId, rncId);
    }

    gsnL3->sgsnStat.numAttachReqRcvd ++;

    memcpy(&reqMsg, &msg[index], sizeof(UmtsLayer3AttachRequest));
    index += sizeof(UmtsLayer3AttachRequest);

    attachType = (UmtsAttachType)reqMsg.attachType;

    // get the imsi from the msg
    imsi.setCompactIMSI(&(msg[index]));

    // to see if the IMSI has been seen
    std::list<UmtsSgsnUeDataInSgsn*>::iterator it;
    for (it = gsnL3->ueInfoList->begin();
        it != gsnL3->ueInfoList->end();
        it ++)
    {
        if ((*it)->imsi == imsi)
        {
            break;
        }
    }

    if (it == gsnL3->ueInfoList->end())
    {
        newUeInfo = UmtsLayer3GsnAddUeInfo(
                        node,
                        umtsL3,
                        gsnL3,
                        ueId,
                        rncId,
                        imsi);
    }
    else
    {
        newUeInfo = *it;
    }

    // allocate TMSI
    // move the GMM status to COMMON-procedure-initated
    newUeInfo->gmmData->mainState =
        UMTS_GMM_NW_COMMON_PROCEDURE_INITIATED;
    newUeInfo->gmmData->counterT3350 = 0;

    // send Attach accept
    Umtslayer3GsnGMmSendAttachAccept(
        node, umtsL3,
        gsnL3, newUeInfo->tmsi,
        (UmtsAttachResult)attachType,
        ueId);

    gsnL3->sgsnStat.numAttachAcptSent ++;

    if (attachType == UMTS_GPRS_ATTACH_ONLY_FOLLOW_ON ||
        attachType == UMTS_COMBINED_ATTACH_FOLLOW_ON)
    {
        newUeInfo->gmmData->followOnProc = TRUE;
    }
    else
    {
        newUeInfo->gmmData->followOnProc = FALSE;
    }

    // start T3350  to wait for ATTACH OCMPLETE
    if (newUeInfo->gmmData->T3350)
    {
        MESSAGE_CancelSelfMsg(node, newUeInfo->gmmData->T3350);
        newUeInfo->gmmData->T3350 = NULL;
    }

    char timerInfo[MAX_STRING_LENGTH];
    int infoSize = 0;
    memcpy(&timerInfo[infoSize], &ueId, sizeof(ueId));
    infoSize += sizeof(ueId);
    timerInfo[infoSize++] = (char)attachType;
    newUeInfo->gmmData->T3350 =
            UmtsLayer3SetTimer(
                node,
                umtsL3,
                UMTS_LAYER3_TIMER_T3350,
                UMTS_LAYER3_TIMER_T3350_VAL,
                NULL,
                timerInfo,
                infoSize);
    newUeInfo->gmmData->pmmState = UMTS_PMM_CONNECTED;
}

// /**
// FUNCTION   :: UmtsLayer3GsnHandleAttachReq
// LAYER      :: Layer 3
// PURPOSE    :: Handle a Mobility Management message from a UE .
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + transactId     : char                   : Transaction ID
// + msg            : char*     : Pointer to the NAS message contents
// + msgSize        : int                    : Message size
// + ueId           : NodeId                 : UE ID
// + rncId          : NodeId                 : RNC ID
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3GsnHandleAttachCompl(
    Node*               node,
    UmtsLayer3Data*     umtsL3,
    unsigned char       transactId,
    char*               msg,
    int                 msgSize,
    NodeId              ueId,
    NodeId              rncId)
{
    UmtsLayer3Gsn *gsnL3 = (UmtsLayer3Gsn *)(umtsL3->dataPtr);
    BOOL attCompFollowOn = FALSE;

    if (DEBUG_GMM && ueId == DEBUG_UE_ID)
    {
        char timeStr[20];
        TIME_PrintClockInSecond(node->getNodeTime(), timeStr);
        printf("%s, SGSN: %u receives a Attach Complete"
            " message from UE: %u, rncId %d\n",
            timeStr, node->nodeId, ueId, rncId);
    }

    // update stat
    gsnL3->sgsnStat.numAttachComplRcvd ++;

    // to see if the IMSI has been seen
    std::list<UmtsSgsnUeDataInSgsn*>::iterator it;
    for (it = gsnL3->ueInfoList->begin();
        it != gsnL3->ueInfoList->end();
        it ++)
    {
        if ((*it)->ueId == ueId)
        {
            break;
        }
    }

    //ERROR_Assert(it != gsnL3->ueInfoList->end(),
    //    "No UeInfo with this ATTACH COMPLETE");
    if (it == gsnL3->ueInfoList->end())
    {
        return;
    }

    // and move the state to registed
    (*it)->gmmData->mainState =
        UMTS_GMM_NW_REGISTERED;
    (*it)->gmmData->regSubState =
        UMTS_GMM_NW_REGISTERED_NORMAL_SERVICE;

    if ((*it)->gmmData->T3350)
    {
        MESSAGE_CancelSelfMsg(node, (*it)->gmmData->T3350);
        (*it)->gmmData->T3350 = NULL;
    }

    //After ATTACH procedure is finished, release the
    //PS signalling
    attCompFollowOn = (BOOL)(*msg);
    if ((*it)->gmmData->followOnProc == FALSE && !attCompFollowOn)
    {
        (*it)->gmmData->pmmState = UMTS_PMM_IDLE;
        UmtsLayer3SgsnSendIuRelease(
            node,
            umtsL3,
            ueId,
            UMTS_LAYER3_CN_DOMAIN_PS);
    }
}

// /**
// FUNCTION   :: Umtslayer3GsnGMmSendServiceAccept
// LAYER      :: Layer3 GMM
// PURPOSE    :: Send a service accept
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data*   : Pointer to L3 data
// + transactId       : unsigned char     : Transaction ID
// + ueId             : NodeId            : UE ID
// RETURN     :: void : NULL
// **/
static
void Umtslayer3GsnGMmSendServiceAccept(Node* node,
                                       UmtsLayer3Data* umtsL3,
                                       unsigned char transId,
                                       NodeId ueId)
{
    UmtsLayer3Gsn *gsnL3 = (UmtsLayer3Gsn*) umtsL3->dataPtr;

    UmtsLayer3GsnSendNasMsg(
        node,
        umtsL3,
        NULL,
        0,
        (char)UMTS_LAYER3_PD_GMM,
        (char)UMTS_GMM_MSG_TYPE_SERVICE_ACCEPT,
        transId,
        ueId);

    // update stat
    gsnL3->sgsnStat.numServiceAcptSent ++;
}

// /**
// FUNCTION   :: UmtsLayer3GsnHandleServiceRequest
// LAYER      :: Layer 3
// PURPOSE    :: Handle a Mobility Management message from a UE .
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + transactId     : unsigned char          : Transaction ID
// + msg            : char*   : Pointer to the NAS message contents
// + msgSize        : int                    : Message size
// + ueId           : NodeId                 : UE ID
// + rncId          : NodeId                 : RNC ID
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3GsnHandleServiceRequest(
        Node*               node,
        UmtsLayer3Data*     umtsL3,
        unsigned char       transactId,
        char*               msg,
        int                 msgSize,
        NodeId              ueId,
        NodeId              rncId)
{
    UmtsLayer3Gsn *gsnL3 = (UmtsLayer3Gsn *)(umtsL3->dataPtr);
    //int index = 0;
    //UmtsSgsnUeDataInSgsn*  newUeInfo;

    UmtsLayer3ServiceRequest reqMsg;
    memcpy(&reqMsg, msg, sizeof(reqMsg));
    UmtsGmmServiceType serviceType =
        (UmtsGmmServiceType)(reqMsg.serviceType);

    if (DEBUG_GMM && DEBUG_UE_ID == ueId)
    {
        char timeStr[20];
        TIME_PrintClockInSecond(node->getNodeTime(), timeStr);
        printf("%s, SGSN: %u receives a SERVICE REQUEST"
            " message from UE: %u, rncId %d\n",
            timeStr, node->nodeId, ueId, rncId);
    }

    // update stat
    gsnL3->sgsnStat.numServiceReqRcvd ++;

    // to see if the IMSI has been seen
    std::list<UmtsSgsnUeDataInSgsn*>::iterator it;
    for (it = gsnL3->ueInfoList->begin();
         it != gsnL3->ueInfoList->end();
         it ++)
    {
        if ((*it)->ueId == ueId)
        {
            break;
        }
    }

    //ERROR_Assert(it != gsnL3->ueInfoList->end(),
    //    "No UeInfo with this SERVICE REQUEST");
    if (it == gsnL3->ueInfoList->end())
    {
        return;
    }

    // and move the state to connected,
    // TODO: this should be moved to
    // the place handling PS signalling connetion setup/release
    if ((*it)->gmmData->pmmState != UMTS_PMM_CONNECTED)
    {
        // service request is to get rrc layer working
        // it may be sent while PMM is connected
        (*it)->gmmData->pmmState = UMTS_PMM_CONNECTED;
    }

    // in case Paging sent, meanwhile UE sent service request
    // STOP T3313, cosider paging response is rcvd.
    if ((*it)->gmmData->T3313)
    {
        MESSAGE_CancelSelfMsg(node, (*it)->gmmData->T3313);
        (*it)->gmmData->T3313 = NULL;
        (*it)->gmmData->counterT3313 = 0;

        // now NW side is ready to handle PDP context
        // send Request PDP Context Activation to UE
        UmtsLayer3GsnSmSendRequestPDPContextActivation(
            node, umtsL3, gsnL3, ueId);
    }
    //Handle paging response
    if (serviceType != UMTS_GMM_SERVICE_TYPE_PAGINGRESP)
    {
        Umtslayer3GsnGMmSendServiceAccept(node, umtsL3, transactId, ueId);
    }
}

// /**
// FUNCTION   :: UmtsLayer3GsnHandleGMmMsg
// LAYER      :: Layer 3
// PURPOSE    :: Handle a GPRS Mobility Management message from a UE .
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msgType        : UmtsGMmMessageType     : Message type
// + transactId     : unsigned char          : Transaction ID
// + msg            : char*     : Pointer to the NAS message contents
// + msgSize        : int                    : Message size
// + ueId           : NodeId                 : UE ID
// + rncId          : NodeId                 : RNC ID
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3GsnHandleGMmMsg(
    Node*               node,
    UmtsLayer3Data*     umtsL3,
    UmtsGMmMessageType   msgType,
    unsigned char       transactId,
    char*               msg,
    int                 msgSize,
    NodeId              ueId,
    NodeId              rncId)
{
    switch (msgType)
    {
        case UMTS_GMM_MSG_TYPE_ATTACH_REQUEST:
        {
            UmtsLayer3GsnHandleAttachReq(
                node,
                umtsL3,
                transactId,
                msg,
                msgSize,
                ueId,
                rncId);
            break;
        }
        case UMTS_GMM_MSG_TYPE_ATTACH_COMPLETE:
        {
            UmtsLayer3GsnHandleAttachCompl(
                node,
                umtsL3,
                transactId,
                msg,
                msgSize,
                ueId,
                rncId);
            break;
        }
        case UMTS_GMM_MSG_TYPE_SERVICE_REQUEST:
        {
            UmtsLayer3GsnHandleServiceRequest(
                node,
                umtsL3,
                transactId,
                msg,
                msgSize,
                ueId,
                rncId);

            break;
        }
        default:
        {
            char errStr[MAX_STRING_LENGTH];
            sprintf(errStr,
                    "UMTS Layer3: SGSN: %d receivers a GMM "
                    "signalling msg with unknown type: %d",
                    node->nodeId,
                    msgType);
            ERROR_ReportWarning(errStr);
        }
    }
}

// /**
// FUNCTION   :: UmtsGMmHandleTimerT3350
// LAYER      :: Layer 3
// PURPOSE    :: Handle a T3350 timer expired msg
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + gsnL3      : UmtsLayer3Gsn *   : UE data
// + msg       : Message*         : Message containing the packet
// RETURN     :: BOOL : msg need to be reused or not
// **/
static
BOOL UmtsGMmHandleTimerT3350(Node *node,
                             UmtsLayer3Data *umtsL3,
                             UmtsLayer3Gsn *gsnL3,
                             Message *msg)
{
    BOOL retVal = FALSE;
    char* timerInfo = MESSAGE_ReturnInfo(msg) + sizeof(UmtsLayer3Timer);
    NodeId ueId;
    memcpy(&ueId,
           timerInfo,
           sizeof(ueId));
    timerInfo += sizeof(ueId);
    UmtsAttachResult attachType = (UmtsAttachResult)(*timerInfo);

    std::list<UmtsSgsnUeDataInSgsn*>::iterator ueNasIter
        = std::find_if(gsnL3->ueInfoList->begin(),
                       gsnL3->ueInfoList->end(),
                       UmtsFindItemByUeId<UmtsSgsnUeDataInSgsn>(ueId));
    UmtsGMmNwData* ueGmm = (*ueNasIter)->gmmData;
    ueGmm->counterT3350 ++;

    if (ueGmm->counterT3350 <= UMTS_LAYER3_TIMER_GMM_MAXCOUNTER)
    {
        Umtslayer3GsnGMmSendAttachAccept(
            node,
            umtsL3,
            gsnL3,
            (*ueNasIter)->tmsi,
            (UmtsAttachResult)attachType,
            ueId);
        ueGmm->T3350 =
            UmtsLayer3SetTimer(
                node,
                umtsL3,
                UMTS_LAYER3_TIMER_T3350,
                UMTS_LAYER3_TIMER_T3350_VAL,
                msg,
                NULL,
                0);
        retVal = TRUE;
    }
    else
    {
        ueGmm->pmmState = UMTS_PMM_DETACHED;
        ueGmm->mainState = UMTS_GMM_NW_DEREGISTERED;
        ueGmm->T3350 = NULL;

        UmtsLayer3SgsnSendIuRelease(
            node,
            umtsL3,
            ueId,
            UMTS_LAYER3_CN_DOMAIN_PS);
    }
    return retVal;
}

// /**
// FUNCTION   :: UmtsLayer3GsnHandleT3255
// LAYER      :: Layer 3
// PURPOSE    :: Handle a T3350 timer expired msg
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + gsnL3      : UmtsLayer3Gsn *   : UE data
// + msg       : Message*         : Message containing the packet
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3GsnHandleT3255(
        Node *node,
        UmtsLayer3Data *umtsL3,
        UmtsLayer3Gsn *gsnL3,
        Message *msg)
{
    NodeId ueId;
    memcpy(&ueId,
           MESSAGE_ReturnInfo(msg) + sizeof(UmtsLayer3Timer),
           sizeof(ueId));

    std::list<UmtsSgsnUeDataInSgsn*>::iterator ueNasIter
        = std::find_if(gsnL3->ueInfoList->begin(),
                       gsnL3->ueInfoList->end(),
                       UmtsFindItemByUeId<UmtsSgsnUeDataInSgsn>(ueId));

    if (ueNasIter != gsnL3->ueInfoList->end())
    {
        UmtsMmNwData* ueMm = (*ueNasIter)->mmData;
        //If there is no onging MM procedure (CM Serv Req from MS)
        // (paging from NW), release CS signal connection
        if (ueMm->mainState == UMTS_MM_NW_IDLE)
        {
            UmtsLayer3SgsnSendIuRelease(
                node,
                umtsL3,
                ueId,
                UMTS_LAYER3_CN_DOMAIN_CS);
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3GsnHandleFlowTimeout
// LAYER      :: Layer3 SM
// PURPOSE    :: Check if any flow has timed out
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// RETURN      : void             : NULL
// **/
static
void UmtsLayer3GsnHandleFlowTimeout(Node* node, UmtsLayer3Data* umtsL3)
{
    UmtsLayer3Gsn *gsnL3 = (UmtsLayer3Gsn *) (umtsL3->dataPtr);
    std::list<UmtsLayer3GgsnPdpInfo*>::iterator it;
    UmtsLayer3GgsnPdpInfo* pdpInfo;
    clocktype curTime = node->getNodeTime();

    for (it = gsnL3->pdpInfoList->begin();
         it != gsnL3->pdpInfoList->end();
         it ++)
    {
        pdpInfo = (*it);
        if (pdpInfo->srcDestType == UMTS_FLOW_MOBILE_ORIGINATED)
        {
            continue;
        }

        if (pdpInfo->classifierInfo->domainInfo ==
            UMTS_LAYER3_CN_DOMAIN_PS)
        {
            // check if sm state is idle?
            if (pdpInfo->status == UMTS_GGSN_PDP_ACTIVE &&
                pdpInfo->lastActiveTime + UMTS_DEFAULT_PS_FLOW_IDLE_TIME <
                curTime)
            {
                UmtsLayer3GsnGtpSendDeletePDPContextRequest(
                    node, umtsL3, gsnL3, pdpInfo->nsapi,
                    pdpInfo->ulTeId, pdpInfo->dlTeId,
                    pdpInfo->sgsnAddr);
            }
            else if (pdpInfo->status == UMTS_GGSN_PDP_REJECTED &&
                pdpInfo->lastActiveTime +
                UMTS_DEFAULT_FLOW_RESTART_INTERVAL <
                curTime)
            {
                // remove it from the pdpInfo List
                // to make those rejected(deu to QoS or Link Failure)
                // applicaiton retry
                UmtsLayer3GgsnPdpInfoFinalize(node, gsnL3, pdpInfo);
                *it = NULL;
            }
        }
    }
    gsnL3->pdpInfoList->remove(NULL);
    return;
}

// /**
// FUNCTION   :: UmtsLayer3GsnHandleCellUpTimer
// LAYER      :: Layer 3
// PURPOSE    :: Handle a HLR CELL UPDATE timer expired msg
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + gsnL3      : UmtsLayer3Gsn *   : UE data
// + msg       : Message*         : Message containing the packet
// RETURN     :: BOOL : msg need to be reused or not
// **/
static
BOOL UmtsLayer3GsnHandleCellUpTimer(
        Node *node,
        UmtsLayer3Data *umtsL3,
        UmtsLayer3Gsn *gsnL3,
        Message *msg)
{
    UInt32 cellId;
    memcpy(&cellId,
           MESSAGE_ReturnInfo(msg) + sizeof(UmtsLayer3Timer),
           sizeof(cellId));
    UmtsCellCacheMap::iterator it
        = gsnL3->cellCacheMap->find(cellId);

    BOOL retVal = FALSE;
    if (!it->second->hlrUpdated)
    {
        UmtsLayer3GsnUpdateHlrCellInfo(
                node,
                umtsL3,
                cellId,
                it->second->nodebId,
                it->second->rncId);

        UmtsLayer3SetTimer(
                node,
                umtsL3,
                UMTS_LAYER3_TIMER_HLR_CELL_UPDATE,
                UMTS_LAYER3_TIMER_HLR_CELL_UPDATE_VAL,
                msg,
                NULL,
                0);
        retVal = TRUE;
    }
    return retVal;
}

// /**
// FUNCTION   :: UmtsLayer3GsnHandleUpdateGuiTimer
// LAYER      :: Layer3
// PURPOSE    :: Update GUI
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + gsnL3     : UmtsLayer3Gsn *         : gsn data
// RETURN      : void             : NULL
// **/
static
void UmtsLayer3GsnHandleUpdateGuiTimer(Node* node,
                                       UmtsLayer3Data* umtsL3,
                                       UmtsLayer3Gsn* gsnL3)
{
    if (!node->guiOption)
    {
        return;
    }
    if (gsnL3->isGGSN)
    {
        double avgThruput;

        avgThruput = gsnL3->ggsnStat.ingressThruputMeas->GetAvgMeas(
                                node->getNodeTime(),
                                UMTS_STAT_TIME_WIMDOW_AVG_TIME);
        GUI_SendRealData(
            node->nodeId,
            gsnL3->ggsnStat.ingressThruputGuiID,
            avgThruput,
            node->getNodeTime());

        avgThruput = gsnL3->ggsnStat.egressThruputMeas->GetAvgMeas(
                                node->getNodeTime(),
                                UMTS_STAT_TIME_WIMDOW_AVG_TIME);
        GUI_SendRealData(
            node->nodeId,
            gsnL3->ggsnStat.egressThruputGuiId,
            avgThruput,
            node->getNodeTime());
    }
    else
    {
        // more
        GUI_SendIntegerData(
                node->nodeId,
                gsnL3->sgsnStat.numRegedUeGuiId,
                gsnL3->sgsnStat.numVlrEntryAdded -
                    gsnL3->sgsnStat.numVlrEntryRemoved,
                node->getNodeTime());
    }
}

////////
// SM
////////
// /**
// FUNCTION   :: UmtsLayer3GsnSmSendActivatePDPContextAccept
// LAYER      :: Layer3 SM
// PURPOSE    :: Send a accept message for PDP Context Activation Req
// PARAMETERS ::
// + node      : Node*                   : Pointer to node.
// + umtsL3    : UmtsLayer3Data*         : Pointer to L3 data
// + gsnL3     : UmtsLayer3Gsn *         : gsn data
// + flowInfo  : UmtsLayer3SgsnFlowInfo* : Info of the SM session
// + rabId     : char                    : Allocated RAB ID
// + ueId      : NodeId                  : Id of the UE
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3GsnSmSendActivatePDPContextAccept(
         Node* node,
         UmtsLayer3Data* umtsL3,
         UmtsLayer3Gsn* gsnL3,
         UmtsLayer3SgsnFlowInfo* flowInfo,
         char rabId,
         NodeId ueId)
{
    char pktBuff[UMTS_MAX_PACKET_BUFFER_SIZE];
    UmtsLayer3ActivatePDPContextAccept *pdpAcptMsg;
    int index = 0;

    if (DEBUG_SM && ueId == DEBUG_UE_ID)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_SM);
        printf("\n\t send a ACTIVATE PDP CONTEXT ACCEPT to UE %d\n",
               flowInfo->ueId);
    }
    memset(pktBuff, 0, UMTS_MAX_PACKET_BUFFER_SIZE);
    pdpAcptMsg = (UmtsLayer3ActivatePDPContextAccept *) &(pktBuff[0]);
    index += sizeof(UmtsLayer3ActivatePDPContextAccept);

    pdpAcptMsg->sapi = rabId;
    // fill the contents
    UmtsLayer3GsnSendNasMsg(
        node,
        umtsL3,
        pktBuff,
        index,
        (char)UMTS_LAYER3_PD_SM,
        (char)UMTS_SM_MSG_TYPE_ACTIVATE_PDP_ACCEPT,
        flowInfo->transId,
        ueId);
}

// /**
// FUNCTION   :: UmtsLayer3GsnSmSendActivatePDPContextReject
// LAYER      :: Layer3 SM
// PURPOSE    :: Send a reject message for a PDP Context Activation Req
// PARAMETERS ::
// + node      : Node*                   : Pointer to node.
// + umtsL3    : UmtsLayer3Data*         : Pointer to L3 data
// + gsnL3     : UmtsLayer3Gsn *         : gsn data
// + flowInfo  : UmtsLayer3SgsnFlowInfo* : Info of the SM session
// + rejectCause   : UmtsSmCauseType     : Reason of the rejection
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3GsnSmSendActivatePDPContextReject(
         Node* node,
         UmtsLayer3Data* umtsL3,
         UmtsLayer3Gsn* gsnL3,
         UmtsLayer3SgsnFlowInfo* flowInfo,
         UmtsSmCauseType rejectCause)
{
    UmtsLayer3ActivatePDPContextReject pdpRjtMsg;

    if (DEBUG_SM && flowInfo->ueId == DEBUG_UE_ID)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_SM);
        printf("\n\t send a ACTIVATE PDP CONTEXT REJECT to UE %d\n",
               flowInfo->ueId);
    }

    memset(&pdpRjtMsg, 0, sizeof(UmtsLayer3ActivatePDPContextReject));

    // fill contents of the message
    pdpRjtMsg.smCause = (unsigned char)rejectCause;

    UmtsLayer3GsnSendNasMsg(
        node,
        umtsL3,
        (char *)&pdpRjtMsg,
        sizeof(UmtsLayer3ActivatePDPContextReject),
        (char)UMTS_LAYER3_PD_SM,
        (char)UMTS_SM_MSG_TYPE_ACTIVATE_PDP_REJECT,
        flowInfo->transId,
        flowInfo->ueId);
}

// /**
// FUNCTION   :: UmtsLayer3GsnSmSendDeactivatePDPContextAccept
// LAYER      :: Layer3 GMM
// PURPOSE    :: Send a Attach request
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data*   : Pointer to L3 data
// + gsnL3            : UmtsLayer3Gsn *   : gsn data
// + srcDestType      : UmtsFlowSrcDestType : app src-dest type
// + transId          : unsigned char     : transaction Id
// + ueId             : NodeId            : Id of the UE
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3GsnSmSendDeactivatePDPContextAccept(
                           Node* node,
                           UmtsLayer3Data* umtsL3,
                           UmtsLayer3Gsn *gsnL3,
                           UmtsFlowSrcDestType srcDestType,
                           unsigned char transId,
                           NodeId ueId)
{
    char pktBuff[UMTS_MAX_PACKET_BUFFER_SIZE];
    int index = 0;

    memset(pktBuff, 0, UMTS_MAX_PACKET_BUFFER_SIZE);


    UmtsLayer3GsnSendNasMsg(
        node,
        umtsL3,
        pktBuff,
        index,
        (char)UMTS_LAYER3_PD_SM,
        (char)UMTS_SM_MSG_TYPE_DEACTIVATE_PDP_ACCEPT,
        transId,
        ueId);
}

// /**
// FUNCTION   :: UmtsLayer3GsnSmSendDeactivatePDPContextRequest
// LAYER      :: Layer3 SM
// PURPOSE    :: Send a Deactivate PDP Context Request
// PARAMETERS ::
// + node      : Node*                    : Pointer to node.
// + umtsL3    : UmtsLayer3Data*          : Pointer to L3 data
// + gsnL3     : UmtsLayer3Gsn *          : gsn data
// + flowInfo  : UmtsLayer3SgsnFlowInfo * : Pointer to flow
// + deactCause: UmtsSmCauseType          : deactivation cause
// RETURN     :: void                     : NULL
// **/
static
void UmtsLayer3GsnSmSendDeactivatePDPContextRequest(
         Node* node,
         UmtsLayer3Data* umtsL3,
         UmtsLayer3Gsn *gsnL3,
         UmtsLayer3SgsnFlowInfo *flowInfo,
         UmtsSmCauseType deactCause)
{
    UmtsLayer3DeactivatePDPContextRequest pdpReqMsg;

    memset(&pdpReqMsg, 0, sizeof(pdpReqMsg));

    pdpReqMsg.smCause = (unsigned char) deactCause;

    flowInfo->cmData.smData->smState = UMTS_SM_NW_PDP_INACTIVE_PENDING;

    UmtsLayer3GsnSendNasMsg(
        node,
        umtsL3,
        (char*)&pdpReqMsg,
        sizeof(pdpReqMsg),
        (char)UMTS_LAYER3_PD_SM,
        (char)UMTS_SM_MSG_TYPE_DEACTIVATE_PDP_REQUEST,
        flowInfo->transId,
        flowInfo->ueId);

    // update statistics
    gsnL3->sgsnStat.numDeactivatePDPContextReqSent ++;

    //Initiate T3395
    char timerInfo[30];
    int index = 0;
    memcpy(&timerInfo[index], &flowInfo->ueId, sizeof(flowInfo->ueId));
    index += sizeof(flowInfo->ueId);
    timerInfo[index ++] = (char)flowInfo->transId;
    timerInfo[index ++] = (char)flowInfo->srcDestType;
    memcpy(&timerInfo[index], &pdpReqMsg, sizeof(pdpReqMsg));
    index += sizeof(pdpReqMsg);
    flowInfo->cmData.smData->T3395 =
                    UmtsLayer3SetTimer(
                        node,
                        umtsL3,
                        UMTS_LAYER3_TIMER_T3395,
                        UMTS_LAYER3_TIMER_T3395_VAL,
                        NULL,
                        (void*)&timerInfo[0],
                        index);
    flowInfo->cmData.smData->counterT3395 = 0;
}

// /**
// FUNCTION   :: UmtsLayer3GsnSmSendRequestPDPContextActivation
// LAYER      :: Layer3 GMM
// PURPOSE    :: Send a Attach request
// PARAMETERS ::
// + node      : Node*             : Pointer to node.
// + umtsL3    : UmtsLayer3Data*   : Pointer to L3 data
// + gsnL3     : UmtsLayer3Gsn *   : gsn data
// + ueId      : UInt32 : The UE which are ready for sending the req
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3GsnSmSendRequestPDPContextActivation(
         Node* node,
         UmtsLayer3Data* umtsL3,
         UmtsLayer3Gsn *gsnL3,
         NodeId ueId)
{
    UmtsLayer3RequestPDPContextActivation reqPdpMsg;
    UmtsSgsnUeDataInSgsn* ueInfo = NULL;
    UmtsLayer3SgsnFlowInfo* flowInfo;

    // find out the pending flows
    std::list<UmtsSgsnUeDataInSgsn*>::iterator itUe;
    for (itUe = gsnL3->ueInfoList->begin();
         itUe != gsnL3->ueInfoList->end();
         itUe ++)
    {
        if ((*itUe)->ueId == ueId)
        {
            // found the ue information
            ueInfo = (*itUe);
            break;
        }
    }

    if (ueInfo == NULL)
    {
        ERROR_ReportWarning("UMTS: Paged UE has no info at SGSN!");
        return;
    }

    // check if the flow already exist
    std::list<UmtsLayer3SgsnFlowInfo*>::iterator itFlow;
    for (itFlow = (*itUe)->appInfoList->begin();
        itFlow != (*itUe)->appInfoList->end();
        itFlow ++)
    {
        if ((*itFlow)->cmData.smData != NULL &&
            (*itFlow)->cmData.smData->smState == UMTS_SM_NW_PAGE_PENDING)
        {
            flowInfo = (*itFlow);

            flowInfo->cmData.smData->smState =
                UMTS_SM_NW_PDP_ACTIVE_PENDING;

            // fill the contents
            memset(&reqPdpMsg, 0, sizeof(reqPdpMsg));
            reqPdpMsg.trafficClass =
                (unsigned char)flowInfo->qosInfo->trafficClass;

            //If REQUEST_PDP_ACTIVATION has not been sent,
            //start sending it now
            if (!flowInfo->cmData.smData->T3385)
            {
                UmtsLayer3GsnSendNasMsg(
                    node,
                    umtsL3,
                    (char*)&reqPdpMsg,
                    sizeof(reqPdpMsg),
                    (char)UMTS_LAYER3_PD_SM,
                    (char)UMTS_SM_MSG_TYPE_REQUEST_PDP_ACTIVATION,
                    flowInfo->transId,
                    ueId);

                // update statistics
                gsnL3->sgsnStat.numReqActivatePDPContextSent ++;

                if (DEBUG_SM && flowInfo->ueId == DEBUG_UE_ID)
                {
                    UmtsLayer3PrintRunTimeInfo(
                        node, UMTS_LAYER3_SUBLAYER_SM);
                    printf("\n\t send a Request PDP CONTEXT ACTIVATION"
                           " to UE: %u, transId %d\n",
                           ueId, flowInfo->transId);
                }

                //Initiate T3385
                char timerInfo[30];
                int index = 0;
                memcpy(&timerInfo[index], &ueId, sizeof(ueId));
                index += sizeof(ueId);
                timerInfo[index ++] = (char)flowInfo->transId;
                timerInfo[index ++] = (char)flowInfo->srcDestType;
                flowInfo->cmData.smData->T3385 =
                                UmtsLayer3SetTimer(
                                    node,
                                    umtsL3,
                                    UMTS_LAYER3_TIMER_T3385,
                                    UMTS_LAYER3_TIMER_T3385_VAL,
                                    NULL,
                                    (void*)&timerInfo[0],
                                    index);
                flowInfo->cmData.smData->counterT3385 = 0;
            } // if (!flowInfo->cmData.smData->T3385)
        } // if ((*itFlow)->cmData.smData))
    } // for (itFlow)
}

// /**
// FUNCTION   :: UmtsLayer3GsnSmHandleActivatePDPContextReq
// LAYER      :: Layer 3
// PURPOSE    :: Handle a Mobility Management message from a UE .
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + transactId     : unsigned char          : Transaction ID
// + msg            : char*   : Pointer to the NAS message contents
// + msgSize        : int                    : Message size
// + ueId           : NodeId                 : UE ID
// + rncId          : NodeId                 : RNC ID
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3GsnSmHandleActivatePDPContextReq(
            Node*               node,
            UmtsLayer3Data*     umtsL3,
            unsigned char        transactId,
            char*               msg,
            int                 msgSize,
            NodeId              ueId,
            NodeId              rncId)
{
    UmtsLayer3Gsn *gsnL3 = (UmtsLayer3Gsn *)(umtsL3->dataPtr);
    int index = 0;
    UmtsLayer3ActivatePDPContextRequest pdpReqMsg;

    unsigned char nsapi;
    UmtsFlowSrcDestType srcDestType;

    if (DEBUG_SM && ueId == DEBUG_UE_ID)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_SM);
        printf("\n\t receives a ACTIVATE PDP CONTEXT REQUEST"
            " from UE: %u, rncId %d transId %d\n",
            ueId, rncId, transactId);
    }

    // update stat
    gsnL3->sgsnStat.numActivatePDPContextReqRcvd ++;

    memcpy(&pdpReqMsg, &msg[index], sizeof(pdpReqMsg));
    index += sizeof(UmtsLayer3ActivatePDPContextRequest);

    if (transactId >= UMTS_LAYER3_NETWORK_MOBILE_START)
    {
        srcDestType = UMTS_FLOW_NETWORK_ORIGINATED;
    }
    else
    {
        srcDestType = UMTS_FLOW_MOBILE_ORIGINATED;
    }

    if (srcDestType == UMTS_FLOW_MOBILE_ORIGINATED)
    {
        NodeAddress ipv4Addr; // pdp dest's addr
        //NodeId pdpNodeId;

        memcpy((char*)&ipv4Addr, &msg[index], pdpReqMsg.pdpAddrLen);
        //pdpNodeId = MAPPING_GetNodeIdFromInterfaceAddress(
        //                                             node, ipv4Addr);
        //if (ueId == pdpNodeId)
    }

    nsapi = pdpReqMsg.nsapi;

    // to see if the IMSI has been seen
    std::list<UmtsSgsnUeDataInSgsn*>::iterator it;
    for (it = gsnL3->ueInfoList->begin();
        it != gsnL3->ueInfoList->end();
        it ++)
    {
        if ((*it)->ueId == ueId)
        {
            break;
        }
    }

    //ERROR_Assert(it != gsnL3->ueInfoList->end(),
    //    "No UeInfo with this activate PDP context request");
    if (it == gsnL3->ueInfoList->end())
    {
        return;
    }
    if ((*it)->gmmData->pmmState != UMTS_PMM_CONNECTED)
    {
        if (DEBUG_SM && ueId == DEBUG_UE_ID)
        {
            UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_SM);
            printf("\n\t PMM is not connected, neglect the request\n");
        }
        return;
    }


    UmtsLayer3SgsnFlowInfo* appInfo = NULL;

    appInfo = UmtsLayer3GsnFindAppInfoFromList(
                  node, gsnL3, (*it)->appInfoList, srcDestType, transactId);

    if (appInfo == NULL)
    {
        // for MS-initate appllicaiton
        // need add appInfo
        UmtsLayer3FlowClassifier appClsInfo;
        appClsInfo.domainInfo = UMTS_LAYER3_CN_DOMAIN_PS;
        appInfo = UmtsLayer3GsnAddAppInfoToList(
            node, gsnL3, (*it)->appInfoList,
            &appClsInfo, srcDestType, transactId, nsapi);
        appInfo->ueId = ueId;

        if (srcDestType == UMTS_FLOW_MOBILE_ORIGINATED)
        {
            // generate a dl tunneling Id
            appInfo->dlTeId = gsnL3->teIdCount ++;

            // get the QoS info
            // traffic class
            UmtsLayer3ConvertOctetToTrafficClass(
                pdpReqMsg.trafficClass,
                pdpReqMsg.trafficHandlingPrio,
                &(appInfo->qosInfo->trafficClass));

            // mx bit rate
            UmtsLayer3ConvertOctetToMaxRate(
                pdpReqMsg.ulMaxBitRate,
                &(appInfo->qosInfo->ulMaxBitRate));
            UmtsLayer3ConvertOctetToMaxRate(
                pdpReqMsg.dlMaxBitRate,
                &(appInfo->qosInfo->dlMaxBitRate));

            UmtsLayer3ConvertOctetToMaxRate(
                pdpReqMsg.ulGuaranteedBitRate,
                &(appInfo->qosInfo->ulGuaranteedBitRate));
            UmtsLayer3ConvertOctetToMaxRate(
                pdpReqMsg.dlGuaranteedBitRate,
                &(appInfo->qosInfo->dlGuaranteedBitRate));


            // delay
            /*if (appInfo->qosInfo->trafficClass <
                UMTS_QOS_CLASS_INTERACTIVE_3)*/
            {
                UmtsLayer3ConvertOctetToDelay(
                    pdpReqMsg.transferDelay,
                    &(appInfo->qosInfo->transferDelay));
            }
            if (DEBUG_QOS)
            {
                char clkStr[MAX_STRING_LENGTH];

                if (appInfo->classifierInfo->domainInfo ==
                    UMTS_LAYER3_CN_DOMAIN_CS)
                {
                    UmtsLayer3PrintRunTimeInfo(
                        node, UMTS_LAYER3_SUBLAYER_CC);
                }
                else if (appInfo->classifierInfo->domainInfo ==
                         UMTS_LAYER3_CN_DOMAIN_PS)
                {
                    UmtsLayer3PrintRunTimeInfo(
                        node, UMTS_LAYER3_SUBLAYER_SM);

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
                     (double)appInfo->qosInfo->sduErrorRatio,
                     appInfo->qosInfo->residualBER);
               printf(" \t transferDelay %s\n",
                     clkStr);
            }
        }
    }
    else if (appInfo->cmData.smData &&
        appInfo->cmData.smData->smState ==
        UMTS_SM_NW_PDP_ACTIVE)
    {
        // Send Accept msg to UE
        UmtsLayer3GsnSmSendActivatePDPContextAccept(node,
                                                    umtsL3,
                                                    gsnL3,
                                                    appInfo,
                                                    appInfo->rabId,
                                                    ueId);

        // update stat
        gsnL3->sgsnStat.numActivatePDPContextAcptSent ++;

        // already exist. for example, a network-initiated session
        if (appInfo->cmData.smData->T3385)
        {
            MESSAGE_CancelSelfMsg(node, appInfo->cmData.smData->T3385);
            appInfo->cmData.smData->T3385 = NULL;
        }

        return;
    }
    // send a create PDP context request
    UmtsLayer3GsnGtpSendCreatePDPContextRequest(
        node, umtsL3, gsnL3, appInfo);

    // update stats
    gsnL3->sgsnStat.numCreatePDPContextReqSent ++;

    // already exist. for example, a network-initiated session
    if (appInfo->cmData.smData->T3385)
    {
        MESSAGE_CancelSelfMsg(node, appInfo->cmData.smData->T3385);
        appInfo->cmData.smData->T3385 = NULL;
    }
}

// /**
// FUNCTION   :: UmtsLayer3GsnSmHandleDeactivatePDPContextReq
// LAYER      :: Layer 3
// PURPOSE    :: Handle a Mobility Management message from a UE .
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + transactId     : unsigned char          : Transaction ID
// + msg            : char*   : Pointer to the NAS message contents
// + msgSize        : int                    : Message size
// + ueId           : NodeId                 : UE ID
// + rncId          : NodeId                 : RNC ID
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3GsnSmHandleDeactivatePDPContextReq(
    Node*               node,
    UmtsLayer3Data*     umtsL3,
    unsigned char        transactId,
    char*               msg,
    int                 msgSize,
    NodeId              ueId,
    NodeId              rncId)
{
    UmtsLayer3Gsn *gsnL3 = (UmtsLayer3Gsn *)(umtsL3->dataPtr);
    int index = 0;
    UmtsFlowSrcDestType srcDestType;
    UmtsLayer3DeactivatePDPContextRequest pdpReqMsg;

    if (DEBUG_SM && ueId == DEBUG_UE_ID)
    {
        char timeStr[20];
        TIME_PrintClockInSecond(node->getNodeTime(), timeStr);
        printf("%s, SGSN: %u receives a DEACTIVATE PDP CONTEXT REQUEST"
            " message from UE: %u, rncId %d\n",
            timeStr, node->nodeId, ueId, rncId);
    }

    // update stat
    gsnL3->sgsnStat.numDeactivatePDPContextReqRcvd ++;

    memcpy(&pdpReqMsg, &msg[index], sizeof(pdpReqMsg));
    index += sizeof(UmtsLayer3DeactivatePDPContextRequest);


    if (transactId >= UMTS_LAYER3_NETWORK_MOBILE_START)
    {
        srcDestType = UMTS_FLOW_NETWORK_ORIGINATED;
    }
    else
    {
        srcDestType = UMTS_FLOW_MOBILE_ORIGINATED;
    }
    // to see if the IMSI has been seen
    std::list<UmtsSgsnUeDataInSgsn*>::iterator it;
    for (it = gsnL3->ueInfoList->begin();
        it != gsnL3->ueInfoList->end();
        it ++)
    {
        if ((*it)->ueId == ueId)
        {
            break;
        }
    }
    //ERROR_Assert(it != gsnL3->ueInfoList->end(),
    //    "no ueInfo link with this Deactivate req");
    if (it == gsnL3->ueInfoList->end())
    {
        return;
    }
    UmtsLayer3SgsnFlowInfo* appInfo = NULL;

    appInfo = UmtsLayer3GsnFindAppInfoFromList(
                  node, gsnL3, (*it)->appInfoList, srcDestType, transactId);

    //ERROR_Assert(appInfo != NULL,
    //    "no appInfo link with this Deactivate request");
    if (appInfo != NULL)
    {
        // send DELETE PDP CONTEXT REQUEST
        UmtsLayer3GsnGtpSendDeletePDPContextRequest(
            node, umtsL3, gsnL3, appInfo->nsapi,
            appInfo->ulTeId, appInfo->dlTeId,
            appInfo->ggsnAddr);
    }
}

// /**
// FUNCTION   :: UmtsLayer3GsnSmHandleDeactivatePDPContextAccept
// LAYER      :: Layer 3
// PURPOSE    :: Handle SM Deactivate PDP Context Accept from UE
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + transactId     : unsigned char          : Transaction ID
// + msg            : char*  : Pointer to the NAS message contents
// + msgSize        : int                    : Message size
// + ueId           : NodeId                 : UE ID
// + rncId          : NodeId                 : RNC ID
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3GsnSmHandleDeactivatePDPContextAccept(
         Node*               node,
         UmtsLayer3Data*     umtsL3,
         unsigned char       transactId,
         char*               msg,
         int                 msgSize,
         NodeId              ueId,
         NodeId              rncId)
{
    UmtsLayer3Gsn *gsnL3 = (UmtsLayer3Gsn *)(umtsL3->dataPtr);
    int index = 0;
    UmtsFlowSrcDestType srcDestType;
    UmtsLayer3DeactivatePDPContextAccept pdpAcptMsg;
    UmtsLayer3SgsnFlowInfo* flowInfo = NULL;

    if (DEBUG_SM && ueId == DEBUG_UE_ID)
    {
        char timeStr[20];
        TIME_PrintClockInSecond(node->getNodeTime(), timeStr);
        printf("%s, SGSN: %u receives a DEACTIVATE PDP CONTEXT ACCEPT"
            " message from UE: %u, rncId %d\n",
            timeStr, node->nodeId, ueId, rncId);
    }

    // update stat
    gsnL3->sgsnStat.numDeactivatePDPContextAcptRcvd ++;

    memcpy(&pdpAcptMsg, &msg[index], sizeof(pdpAcptMsg));
    index += sizeof(UmtsLayer3DeactivatePDPContextAccept);

    if (transactId >= UMTS_LAYER3_NETWORK_MOBILE_START)
    {
        srcDestType = UMTS_FLOW_NETWORK_ORIGINATED;
    }
    else
    {
        srcDestType = UMTS_FLOW_MOBILE_ORIGINATED;
    }

    // to see if the IMSI has been seen
    std::list<UmtsSgsnUeDataInSgsn*>::iterator it;
    for (it = gsnL3->ueInfoList->begin();
        it != gsnL3->ueInfoList->end();
        it ++)
    {
        if ((*it)->ueId == ueId)
        {
            break;
        }
    }
    //ERROR_Assert(it != gsnL3->ueInfoList->end(),
    //    "no ueInfo link with this Deactivate accept");
    if (it == gsnL3->ueInfoList->end())
    {
        return;
    }

    flowInfo = UmtsLayer3GsnFindAppInfoFromList(
                  node, gsnL3, (*it)->appInfoList, srcDestType, transactId);

    //ERROR_Assert(flowInfo != NULL,
    //    "no flowInfo link with this Deactivate accept");
    if (flowInfo == NULL)
    {
        return;
    }

    // send Delete PDP Context Response to GGSN
    UmtsLayer3GsnGtpSendDeletePDPContextResponse(
        node,
        umtsL3,
        gsnL3,
        flowInfo->nsapi,
        (unsigned char)flowInfo->ulTeId,
        (unsigned char)flowInfo->dlTeId,
        UMTS_GTP_REQUEST_ACCEPTED,
        flowInfo->ggsnAddr);

    // cancel pending timers
    if (flowInfo->cmData.smData->smState == UMTS_SM_NW_PDP_INACTIVE_PENDING)
    {
        // if UE init deactivation and NW init DEACTIVATIOn collide
        // cancel T3395
        if (flowInfo->cmData.smData->T3395)
        {
            MESSAGE_CancelSelfMsg(node, flowInfo->cmData.smData->T3395);
            flowInfo->cmData.smData->T3395 = NULL;
        }
    }

    // move state to inactive
    flowInfo->cmData.smData->smState = UMTS_SM_NW_PDP_INACTIVE;

    //Tell the RNC to release RAB
    UmtsLayer3SgsnSendGtpRabRelease(
        node,
        umtsL3,
        flowInfo->ueId,
        flowInfo->rabId,
        rncId);

    // remove the flow info from list
    UmtsLayer3GsnRemoveAppInfoFromList(
        node, gsnL3, (*it)->transIdBitSet, (*it)->appInfoList,
        flowInfo->srcDestType, flowInfo->transId);

    //Check if need to release PS signalling connection
    if (0 == std::count_if(
            (*it)->appInfoList->begin(),
            (*it)->appInfoList->end(),
            UmtsCheckFlowDomain(UMTS_LAYER3_CN_DOMAIN_PS)))
    {
        (*it)->gmmData->pmmState = UMTS_PMM_IDLE;
        UmtsLayer3SgsnSendIuRelease(
            node,
            umtsL3,
            ueId,
            UMTS_LAYER3_CN_DOMAIN_PS);
    }
}

// /**
// FUNCTION   :: UmtsLayer3GsnHandleT3313
// LAYER      :: Layer 3
// PURPOSE    :: Handle a T3313 timer expired msg
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + gsnL3     : UmtsLayer3Gsn *  : UE data
// + msg       : Message*         : Message containing the packet
// RETURN     :: void : NULL
// **/
static
BOOL UmtsLayer3GsnHandleT3313(
        Node *node,
        UmtsLayer3Data *umtsL3,
        UmtsLayer3Gsn *gsnL3,
        Message *msg)
{
    BOOL msgReuse = FALSE;
    char* timerInfo = MESSAGE_ReturnInfo(msg) + sizeof(UmtsLayer3Timer);
    NodeId ueId;
    UmtsPagingCause cause;

    int index = 0;
    memcpy(&ueId, &timerInfo[index], sizeof(ueId));
    index += sizeof(ueId);
    cause = (UmtsPagingCause)(timerInfo[index ++]);

    std::list<UmtsSgsnUeDataInSgsn*>::iterator ueNasIt;
    ueNasIt = std::find_if(
                gsnL3->ueInfoList->begin(),
                gsnL3->ueInfoList->end(),
                UmtsFindItemByUeId<UmtsSgsnUeDataInSgsn>(ueId));

    if (ueNasIt != gsnL3->ueInfoList->end())
    {
        (*ueNasIt)->gmmData->counterT3313 ++;
        (*ueNasIt)->gmmData->T3313 = NULL;

        if ((*ueNasIt)->gmmData->counterT3313 >
            UMTS_LAYER3_TIMER_GMM_MAXCOUNTER)
        {
            (*ueNasIt)->gmmData->counterT3313 = 0;
            std::list<UmtsLayer3SgsnFlowInfo*>::iterator flowIter;
            for (flowIter = (*ueNasIt)->appInfoList->begin();
                 flowIter != (*ueNasIt)->appInfoList->end();
                 ++flowIter)
            {
                UmtsLayer3SgsnFlowInfo* flowInfo = *flowIter;

                if (flowInfo->cmData.smData->smState ==
                    UMTS_SM_NW_PAGE_PENDING)
                {
                    // Reject the PDP
                    UmtsLayer3GsnGtpSendPduNotificationResponse(
                        node,
                        umtsL3,
                        gsnL3,
                        *ueNasIt,
                        flowInfo,
                        UMTS_GTP_REQUEST_REJECTED,
                        gsnL3->ggsnAddr);

                    gsnL3->sgsnStat.numPduNotificationRspSent ++;

                    // move state to inactive
                    flowInfo->cmData.smData->smState =
                        UMTS_SM_NW_PDP_REJECTED;

                    if ((*flowIter)->srcDestType ==
                        UMTS_FLOW_NETWORK_ORIGINATED)
                    {
                        (*ueNasIt)->transIdBitSet->reset(
                            (*flowIter)->transId -
                            UMTS_LAYER3_NETWORK_MOBILE_START);
                    }
                    UmtsLayer3GsnAppInfoFinalize(node, gsnL3, *flowIter);
                    *flowIter = NULL;
                }
            }
            (*ueNasIt)->appInfoList->remove(NULL);
            //Check if need to release PS signalling connection
            if (0 == std::count_if(
                    (*ueNasIt)->appInfoList->begin(),
                    (*ueNasIt)->appInfoList->end(),
                    UmtsCheckFlowDomain(UMTS_LAYER3_CN_DOMAIN_PS)))
            {
                if ((*ueNasIt)->gmmData->pmmState == UMTS_PMM_CONNECTED)
                {
                    (*ueNasIt)->gmmData->pmmState = UMTS_PMM_IDLE;
                    UmtsLayer3SgsnSendIuRelease(
                        node,
                        umtsL3,
                        (*ueNasIt)->ueId,
                        UMTS_LAYER3_CN_DOMAIN_PS);
                }
            }
        }
        else
        {
            (*ueNasIt)->gmmData->T3313 =
                                UmtsLayer3SetTimer(
                                    node,
                                    umtsL3,
                                    UMTS_LAYER3_TIMER_T3385,
                                    UMTS_LAYER3_TIMER_T3385_VAL,
                                    msg,
                                    NULL,
                                    0);
            UmtsLayer3SgsnSendPaging(
                node,
                umtsL3,
                ueId,
                UMTS_LAYER3_CN_DOMAIN_PS,
                cause);
             msgReuse = TRUE;
        }
    }
    return msgReuse;
}


// /**
// FUNCTION   :: UmtsLayer3GsnHandleT3385
// LAYER      :: Layer 3
// PURPOSE    :: Handle a T3385 timer expired msg
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + gsnL3      : UmtsLayer3Gsn *   : UE data
// + msg       : Message*         : Message containing the packet
// RETURN     :: void : NULL
// **/
static
BOOL UmtsLayer3GsnHandleT3385(
        Node *node,
        UmtsLayer3Data *umtsL3,
        UmtsLayer3Gsn *gsnL3,
        Message *msg)
{
    BOOL msgReuse = FALSE;
    char* timerInfo = MESSAGE_ReturnInfo(msg) + sizeof(UmtsLayer3Timer);
    NodeId ueId;
    UmtsFlowSrcDestType srcDestType;
    unsigned char  transId;

    int index = 0;
    memcpy(&ueId, &timerInfo[index], sizeof(ueId));
    index += sizeof(ueId);
    transId = timerInfo[index ++];
    srcDestType = (UmtsFlowSrcDestType)(timerInfo[index ++]);

    std::list<UmtsSgsnUeDataInSgsn*>::iterator ueNasIt;
    ueNasIt = std::find_if(
                gsnL3->ueInfoList->begin(),
                gsnL3->ueInfoList->end(),
                UmtsFindItemByUeId<UmtsSgsnUeDataInSgsn>(ueId));

    if (ueNasIt != gsnL3->ueInfoList->end() &&
            (*ueNasIt)->gmmData->pmmState == UMTS_PMM_CONNECTED)
    {
        UmtsLayer3SgsnFlowInfo* flowInfo =
            UmtsLayer3GsnFindAppInfoFromList(
                    node,
                    gsnL3,
                    (*ueNasIt)->appInfoList,
                    srcDestType,
                    transId);
        if (flowInfo && flowInfo->cmData.smData->smState ==
                            UMTS_SM_NW_PDP_ACTIVE_PENDING)
        {
            UmtsSmNwData* ueSm = flowInfo->cmData.smData;
            ueSm->counterT3385++;
            if (ueSm->counterT3385 > UMTS_LAYER3_TIMER_SM_MAXCOUNTER)
            {
                //Abort the procedure
                flowInfo->cmData.smData->smState =
                                UMTS_SM_NW_PDP_REJECTED;
                ueSm->T3385 = NULL;
                //NOTIFY the GGSN
                UmtsLayer3GsnGtpSendPduNotificationResponse(
                    node,
                    umtsL3,
                    gsnL3,
                    *ueNasIt,
                    flowInfo,
                    UMTS_GTP_REQUEST_REJECTED,
                    gsnL3->ggsnAddr);

                gsnL3->sgsnStat.numPduNotificationRspSent ++;
                // remove the flow info from list
                UmtsLayer3GsnRemoveAppInfoFromList(
                    node,
                    gsnL3,
                    (*ueNasIt)->transIdBitSet,
                    (*ueNasIt)->appInfoList,
                    flowInfo->srcDestType,
                    flowInfo->transId);

                //Check if need to release PS signalling connection
                if (0 == std::count_if(
                        (*ueNasIt)->appInfoList->begin(),
                        (*ueNasIt)->appInfoList->end(),
                        UmtsCheckFlowDomain(UMTS_LAYER3_CN_DOMAIN_PS)))
                {
                    (*ueNasIt)->gmmData->pmmState = UMTS_PMM_IDLE;
                    UmtsLayer3SgsnSendIuRelease(
                        node,
                        umtsL3,
                        (*ueNasIt)->ueId,
                        UMTS_LAYER3_CN_DOMAIN_PS);
                }
            }
            else
            {
                //resend REQUEST_PDP_ACTIVATION
                UmtsLayer3RequestPDPContextActivation reqPdpMsg;
                memset(&reqPdpMsg, 0, sizeof(reqPdpMsg));
                reqPdpMsg.trafficClass =
                    (unsigned char)flowInfo->qosInfo->trafficClass;
                UmtsLayer3GsnSendNasMsg(
                    node,
                    umtsL3,
                    (char*)&reqPdpMsg,
                    sizeof(reqPdpMsg),
                    (char)UMTS_LAYER3_PD_SM,
                    (char)UMTS_SM_MSG_TYPE_REQUEST_PDP_ACTIVATION,
                    flowInfo->transId,
                    ueId);

                // update statistics
                gsnL3->sgsnStat.numReqActivatePDPContextSent ++;

                if (DEBUG_SM && flowInfo->ueId == DEBUG_UE_ID)
                {
                    UmtsLayer3PrintRunTimeInfo(
                        node, UMTS_LAYER3_SUBLAYER_SM);
                    printf("\n\t send a Request PDP CONTEXT ACTIVATION"
                           " to UE: %u, transId %d\n",
                           ueId, flowInfo->transId);
                }

                flowInfo->cmData.smData->T3385 =
                                UmtsLayer3SetTimer(
                                    node,
                                    umtsL3,
                                    UMTS_LAYER3_TIMER_T3385,
                                    UMTS_LAYER3_TIMER_T3385_VAL,
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
// FUNCTION   :: UmtsLayer3GsnHandleT3395
// LAYER      :: Layer 3
// PURPOSE    :: Handle a T3395 timer expired msg
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + gsnL3     : UmtsLayer3Gsn *  : UE data
// + msg       : Message*         : Message containing the packet
// RETURN     :: void : NULL
// **/
static
BOOL UmtsLayer3GsnHandleT3395(
        Node *node,
        UmtsLayer3Data *umtsL3,
        UmtsLayer3Gsn *gsnL3,
        Message *msg)
{
    BOOL msgReuse = FALSE;
    char* timerInfo = MESSAGE_ReturnInfo(msg) + sizeof(UmtsLayer3Timer);
    NodeId ueId;
    UmtsFlowSrcDestType srcDestType;
    unsigned char  transId;
    UmtsLayer3DeactivatePDPContextRequest pdpReqMsg;

    int index = 0;
    memcpy(&ueId, &timerInfo[index], sizeof(ueId));
    index += sizeof(ueId);
    transId = timerInfo[index ++];
    srcDestType = (UmtsFlowSrcDestType)(timerInfo[index ++]);
    memcpy(&pdpReqMsg, &timerInfo[index], sizeof(pdpReqMsg));

    std::list<UmtsSgsnUeDataInSgsn*>::iterator ueNasIt;
    ueNasIt = std::find_if(
                gsnL3->ueInfoList->begin(),
                gsnL3->ueInfoList->end(),
                UmtsFindItemByUeId<UmtsSgsnUeDataInSgsn>(ueId));

    if (ueNasIt != gsnL3->ueInfoList->end() &&
            (*ueNasIt)->gmmData->pmmState == UMTS_PMM_CONNECTED)
    {
        UmtsLayer3SgsnFlowInfo* flowInfo =
            UmtsLayer3GsnFindAppInfoFromList(
                    node,
                    gsnL3,
                    (*ueNasIt)->appInfoList,
                    srcDestType,
                    transId);
        flowInfo->cmData.smData->T3395 = NULL;
        if (flowInfo && flowInfo->cmData.smData->smState ==
                                UMTS_SM_NW_PDP_INACTIVE_PENDING)
        {
            UmtsSmNwData* ueSm = flowInfo->cmData.smData;
            ueSm->counterT3395++;
            if (ueSm->counterT3395 > UMTS_LAYER3_TIMER_SM_MAXCOUNTER)
            {
                //Delete the PDP
                UmtsLayer3GsnGtpSendDeletePDPContextResponse(
                    node,
                    umtsL3,
                    gsnL3,
                    flowInfo->nsapi,
                    (unsigned char)flowInfo->ulTeId,
                    (unsigned char)flowInfo->dlTeId,
                    UMTS_GTP_REQUEST_ACCEPTED,
                    flowInfo->ggsnAddr);
                // move state to inactive
                flowInfo->cmData.smData->smState = UMTS_SM_NW_PDP_INACTIVE;

                NodeId rncId;
                UInt32 cellId;
                BOOL found = UmtsLayer3GsnLookupVlr(node,
                                                    umtsL3,
                                                    flowInfo->ueId,
                                                    &rncId,
                                                    &cellId);
                ERROR_Assert(found != FALSE, "Cannot find UE in my VLR!");
                //Tell the RNC to release RAB
                UmtsLayer3SgsnSendGtpRabRelease(
                    node,
                    umtsL3,
                    flowInfo->ueId,
                    flowInfo->rabId,
                    rncId);

                // remove the flow info from list
                UmtsLayer3GsnRemoveAppInfoFromList(
                    node,
                    gsnL3,
                    (*ueNasIt)->transIdBitSet,
                    (*ueNasIt)->appInfoList,
                    flowInfo->srcDestType,
                    flowInfo->transId);

                //Check if need to release PS signalling connection
                if (0 == std::count_if(
                        (*ueNasIt)->appInfoList->begin(),
                        (*ueNasIt)->appInfoList->end(),
                        UmtsCheckFlowDomain(UMTS_LAYER3_CN_DOMAIN_PS)))
                {
                    (*ueNasIt)->gmmData->pmmState = UMTS_PMM_IDLE;
                    UmtsLayer3SgsnSendIuRelease(
                        node,
                        umtsL3,
                        (*ueNasIt)->ueId,
                        UMTS_LAYER3_CN_DOMAIN_PS);
                }
            }
            else
            {
                //resend REQUEST_PDP_DEACTIVATION
                UmtsLayer3GsnSendNasMsg(
                    node,
                    umtsL3,
                    (char*)&pdpReqMsg,
                    sizeof(pdpReqMsg),
                    (char)UMTS_LAYER3_PD_SM,
                    (char)UMTS_SM_MSG_TYPE_DEACTIVATE_PDP_REQUEST,
                    flowInfo->transId,
                    flowInfo->ueId);
                flowInfo->cmData.smData->T3395 =
                                UmtsLayer3SetTimer(
                                    node,
                                    umtsL3,
                                    UMTS_LAYER3_TIMER_T3395,
                                    UMTS_LAYER3_TIMER_T3395_VAL,
                                    msg,
                                    NULL,
                                    0);
                msgReuse = TRUE;
            } // if (ueSm->counterT3395
        } // if (flowInfo)
    } // if (ueNasIt)
    return msgReuse;
}


// /**
// FUNCTION   :: UmtsLayer3GsnHandleSmMsg
// LAYER      :: Layer 3
// PURPOSE    :: Handle a SM message from a UE .
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msgType        : UmtsSmMessageType      : Message type
// + transactId     : char                   : Transaction ID
// + msg            : char* : Pointer to the NAS message contents
// + msgSize        : int                    : Message size
// + ueId           : NodeId                 : UE ID
// + rncId          : NodeId                 : RNC ID
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3GsnHandleSmMsg(
    Node*               node,
    UmtsLayer3Data*     umtsL3,
    UmtsSmMessageType   msgType,
    unsigned char       transactId,
    char*               msg,
    int                 msgSize,
    NodeId              ueId,
    NodeId              rncId)
{

    switch (msgType)
    {
        case UMTS_SM_MSG_TYPE_ACTIVATE_PDP_REQUEST:
        {
            UmtsLayer3GsnSmHandleActivatePDPContextReq(
                node,
                umtsL3,
                transactId,
                msg,
                msgSize,
                ueId,
                rncId);
            break;
        }
        case UMTS_SM_MSG_TYPE_DEACTIVATE_PDP_REQUEST:
        {
            UmtsLayer3GsnSmHandleDeactivatePDPContextReq(
                node,
                umtsL3,
                transactId,
                msg,
                msgSize,
                ueId,
                rncId);
            break;
        }
        case UMTS_SM_MSG_TYPE_DEACTIVATE_PDP_ACCEPT:
        {
            UmtsLayer3GsnSmHandleDeactivatePDPContextAccept(
                node,
                umtsL3,
                transactId,
                msg,
                msgSize,
                ueId,
                rncId);
            break;
        }
        default:
        {
            char errStr[MAX_STRING_LENGTH];
            sprintf(errStr,
                    "UMTS Layer3: SGSN: %d receivers a SM "
                    "signalling msg with unknown type: %d",
                    node->nodeId,
                    msgType);
            ERROR_ReportWarning(errStr);
        }
    }
}

///***************************************************************
// CC message sending and handling
///***************************************************************
// /**
// FUNCTION   :: UmtsLayer3MscSendCallProcToUe
// LAYER      :: Layer 3
// PURPOSE    :: Send a CC CALL PROCEEDING message to the UE
// PARAMETERS ::
// + node           : Node*                 : Pointer to node.
// + gsnL3          : UmtsLayer3Gsn*        : Pointer to the GSN Layer3 data
// + ueNas          : UmtsSgsnUeDataInSgsn* : Pointer to the UE info data
// + ueFlow         : UmtsLayer3SgsnFlowInfo* : app info
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3MscSendCallProcToUe(
    Node*                   node,
    UmtsLayer3Data*         umtsL3,
    UmtsSgsnUeDataInSgsn*   ueNas,
    UmtsLayer3SgsnFlowInfo* ueFlow)
{
    UmtsLayer3Gsn* gsnL3 = (UmtsLayer3Gsn*)(umtsL3->dataPtr);
    gsnL3->sgsnStat.numCcCallProcSent++;

    UmtsLayer3GsnSendNasMsg(
        node,
        umtsL3,
        NULL,
        0,
        (char)UMTS_LAYER3_PD_CC,
        (char)UMTS_CC_MSG_TYPE_CALL_PROCEEDING,
        ueFlow->transId,
        ueNas->ueId);
}

// /**
// FUNCTION   :: UmtsLayer3MscSendAlertingToUe
// LAYER      :: Layer 3
// PURPOSE    :: Send a CC ALERTING message to the UE
// PARAMETERS ::
// + node           : Node*                 : Pointer to node.
// + umtsL3         : UmtsLayer3Data*       : Pointer to the GSN Layer3 data
// + ueNas          : UmtsSgsnUeDataInSgsn* : Pointer to the UE info data
// + ueFlow         : UmtsLayer3SgsnFlowInfo* : app info
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3MscSendAlertingToUe(
    Node*                   node,
    UmtsLayer3Data*         umtsL3,
    UmtsSgsnUeDataInSgsn*   ueNas,
    UmtsLayer3SgsnFlowInfo* ueFlow)
{
    UmtsLayer3Gsn* gsnL3 = (UmtsLayer3Gsn*)(umtsL3->dataPtr);
    gsnL3->sgsnStat.numCcAlertingSent++;

    UmtsLayer3GsnSendNasMsg(
        node,
        umtsL3,
        NULL,
        0,
        (char)UMTS_LAYER3_PD_CC,
        (char)UMTS_CC_MSG_TYPE_ALERTING,
        ueFlow->transId,
        ueNas->ueId);
}

// /**
// FUNCTION   :: UmtsLayer3MscSendConnectToUe
// LAYER      :: Layer 3
// PURPOSE    :: Send a CC CONNECT message to the UE
// PARAMETERS ::
// + node           : Node*                 : Pointer to node.
// + umtsL3         : UmtsLayer3Data*       : Pointer to the GSN Layer3 data
// + ueNas          : UmtsSgsnUeDataInSgsn* : Pointer to the UE info data
// + ueFlow         : UmtsLayer3SgsnFlowInfo* : app info
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3MscSendConnectToUe(
    Node*                   node,
    UmtsLayer3Data*         umtsL3,
    UmtsSgsnUeDataInSgsn*   ueNas,
    UmtsLayer3SgsnFlowInfo* ueFlow)
{
    UmtsLayer3Gsn* gsnL3 = (UmtsLayer3Gsn*)(umtsL3->dataPtr);
    gsnL3->sgsnStat.numCcConnectSent++;

    char msg[1];
    msg[0] = ueFlow->rabId;
    UmtsLayer3GsnSendNasMsg(
        node,
        umtsL3,
        msg,
        1,
        (char)UMTS_LAYER3_PD_CC,
        (char)UMTS_CC_MSG_TYPE_CONNECT,
        ueFlow->transId,
        ueNas->ueId);
}

// /**
// FUNCTION   :: UmtsLayer3MscSendMobTermCallSetupToUe
// LAYER      :: Layer 3
// PURPOSE    :: Send a SETUP message to the UE to establish a mobile
//               terminating call
// PARAMETERS ::
// + node        : Node*                  : Pointer to node.
// + umtsL3      : UmtsLayer3Data*        : Pointer to the GSN Layer3 data
// + ueNas       : UmtsSgsnUeDataInSgsn*  : Pointer to the UE info data
// + ueFlow      : UmtsLayer3SgsnFlowInfo*: Pointer to the application flow
// RETURN        :: void : NULL
// **/
static
void UmtsLayer3MscSendMobTermCallSetupToUe(
    Node*                   node,
    UmtsLayer3Data*         umtsL3,
    UmtsSgsnUeDataInSgsn*   ueNas,
    UmtsLayer3SgsnFlowInfo* ueFlow)
{
    UmtsLayer3Gsn* gsnL3 = (UmtsLayer3Gsn*)(umtsL3->dataPtr);
    gsnL3->sgsnStat.numCcSetupSent++;

    char msg[40];
    int index = 0;
    UmtsCcNwData* ueCc = ueFlow->cmData.ccData;
    memcpy(msg, &(ueCc->peerAddr), sizeof(ueCc->peerAddr));
    index += sizeof(ueCc->peerAddr);
    memcpy(&msg[index], &(ueCc->callAppId), sizeof(ueCc->callAppId));
    index += sizeof(ueCc->callAppId);
    memcpy(&msg[index], &(ueCc->endTime), sizeof(ueCc->endTime));
    index += sizeof(ueCc->endTime);
    memcpy(&msg[index], &(ueCc->avgTalkTime), sizeof(ueCc->avgTalkTime));
    index += sizeof(ueCc->avgTalkTime);
    UmtsLayer3GsnSendNasMsg(
        node,
        umtsL3,
        msg,
        index,
        (char)UMTS_LAYER3_PD_CC,
        (char)UMTS_CC_MSG_TYPE_SETUP,
        ueFlow->transId,
        ueNas->ueId);

    //Initiate timer T303
    char timerInfo[5];
    memcpy(timerInfo, &(ueNas->ueId), sizeof(ueNas->ueId));
    index = sizeof(ueNas->ueId);
    timerInfo[index++] = ueFlow->transId;
    ueCc->T303 = UmtsLayer3SetTimer(
                        node,
                        umtsL3,
                        UMTS_LAYER3_TIMER_CC_T303,
                        UMTS_LAYER3_TIMER_CC_T303_NW_VAL,
                        NULL,
                        (void*)&timerInfo[0],
                        index);
}

// /**
// FUNCTION   :: UmtsLayer3MscSendConnAckToUe
// LAYER      :: Layer 3
// PURPOSE    :: Send a CONNECT ACKNOWLEDGE message to the UE to establish a
//               mobile terminating call
// PARAMETERS ::
// + node        : Node*                  : Pointer to node.
// + umtsL3      : UmtsLayer3Data*        : Pointer to the GSN Layer3 data
// + ueNas       : UmtsSgsnUeDataInSgsn*  : Pointer to the UE info data
// + ueFlow      : UmtsLayer3SgsnFlowInfo*: Pointer to the application flow
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3MscSendConnAckToUe(
    Node*                   node,
    UmtsLayer3Data*         umtsL3,
    UmtsSgsnUeDataInSgsn*   ueNas,
    UmtsLayer3SgsnFlowInfo* ueFlow)
{
    UmtsLayer3Gsn* gsnL3 = (UmtsLayer3Gsn*)(umtsL3->dataPtr);
    gsnL3->sgsnStat.numCcConnAckSent++;

    char msg[20];
    int index = 0;
    //UmtsCcNwData* ueCc = ueFlow->cmData.ccData;
    memcpy(msg, &(ueFlow->rabId), sizeof(ueFlow->rabId));
    index += sizeof(ueFlow->rabId);
    UmtsLayer3GsnSendNasMsg(
        node,
        umtsL3,
        msg,
        index,
        (char)UMTS_LAYER3_PD_CC,
        (char)UMTS_CC_MSG_TYPE_CONNECT_ACKNOWLEDGE,
        ueFlow->transId,
        ueNas->ueId);
}

// /**
// FUNCTION   :: UmtsLayer3MscSendDisconnectToUe
// LAYER      :: Layer 3
// PURPOSE    :: Send a DISCONNECT message to the UE
// PARAMETERS ::
// + node       : Node*                  : Pointer to node.
// + umtsL3     : UmtsLayer3Data*        : Pointer to the GSN Layer3 data
// + ueNas      : UmtsSgsnUeDataInSgsn*  : Pointer to the UE info data
// + ueFlow     : UmtsLayer3SgsnFlowInfo*: Pointer to the application flow
// + clearType  : UmtsCallClearType      : Call clear type
// RETURN       :: void : NULL
// **/
static
void UmtsLayer3MscSendDisconnectToUe(
    Node*                   node,
    UmtsLayer3Data*         umtsL3,
    UmtsSgsnUeDataInSgsn*   ueNas,
    UmtsLayer3SgsnFlowInfo* ueFlow,
    UmtsCallClearingType    clearType)
{
    UmtsLayer3Gsn* gsnL3 = (UmtsLayer3Gsn*)(umtsL3->dataPtr);
    gsnL3->sgsnStat.numCcDisconnectSent++;

    char packet[4];
    *packet = (char)clearType;
    UmtsLayer3GsnSendNasMsg(
        node,
        umtsL3,
        packet,
        sizeof(char),
        (char)UMTS_LAYER3_PD_CC,
        (char)UMTS_CC_MSG_TYPE_DISCONNECT,
        ueFlow->transId,
        ueNas->ueId);
}

// /**
// FUNCTION   :: UmtsLayer3MscSendReleaseToUe
// LAYER      :: Layer 3
// PURPOSE    :: Send a RELEASE message to the UE
// PARAMETERS ::
// + node       : Node*                  : Pointer to node.
// + umtsL3     : UmtsLayer3Data*        : Pointer to the GSN Layer3 data
// + ueNas      : UmtsSgsnUeDataInSgsn*  : Pointer to the UE info data
// + ueFlow     : UmtsLayer3SgsnFlowInfo*: Pointer to the application flow
// + clearType  : UmtsCallClearType      : Call clear type
// RETURN       :: void : NULL
// **/
static
void UmtsLayer3MscSendReleaseToUe(
        Node*                   node,
        UmtsLayer3Data*         umtsL3,
        UmtsSgsnUeDataInSgsn*   ueNas,
        UmtsLayer3SgsnFlowInfo* ueFlow,
        UmtsCallClearingType    clearType = UMTS_CALL_CLEAR_END)
{
    UmtsLayer3Gsn* gsnL3 = (UmtsLayer3Gsn*)(umtsL3->dataPtr);
    gsnL3->sgsnStat.numCcReleaseSent++;

    char packet[4];
    packet[0] = (char)clearType;
    UmtsLayer3GsnSendNasMsg(
        node,
        umtsL3,
        packet,
        sizeof(char),
        (char)UMTS_LAYER3_PD_CC,
        (char)UMTS_CC_MSG_TYPE_RELEASE,
        ueFlow->transId,
        ueNas->ueId);
}

// /**
// FUNCTION   :: UmtsLayer3MscSendReleaseCompleteToUe
// LAYER      :: Layer 3
// PURPOSE    :: Send a RELEASE COMPLETE message to the UE
// PARAMETERS ::
// + node       : Node*                  : Pointer to node.
// + umtsL3     : UmtsLayer3Data*        : Pointer to the GSN Layer3 data
// + ueNas      : UmtsSgsnUeDataInSgsn*  : Pointer to the UE info data
// + ueFlow     : UmtsLayer3SgsnFlowInfo*: Pointer to the application flow
// RETURN       :: void : NULL
// **/
static
void UmtsLayer3MscSendReleaseCompleteToUe(
        Node*                   node,
        UmtsLayer3Data*         umtsL3,
        UmtsSgsnUeDataInSgsn*   ueNas,
        UmtsLayer3SgsnFlowInfo* ueFlow)
{
    UmtsLayer3Gsn* gsnL3 = (UmtsLayer3Gsn*)(umtsL3->dataPtr);
    gsnL3->sgsnStat.numCcReleaseCompleteSent++;

    UmtsLayer3GsnSendNasMsg(
        node,
        umtsL3,
        NULL,
        0,
        (char)UMTS_LAYER3_PD_CC,
        (char)UMTS_CC_MSG_TYPE_RELEASE_COMPLETE,
        ueFlow->transId,
        ueNas->ueId);
}

// /**
// FUNCTION   :: UmtsLayer3MscInitCallClearing
// LAYER      :: Layer 3
// PURPOSE    :: Initiate Call clearing procedure from the network
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data*        : Pointer to the GSN Layer3 data
// + ueNas     : UmtsSgsnUeDataInSgsn*  : Pointer to the UE info data
// + ueFlow    : UmtsLayer3SgsnFlowInfo*: Pointer to the application flow
// + clearType : UmtsCallClearType      : Call clear type
// RETURN      :: void : NULL
// **/
static
void UmtsLayer3MscInitCallClearing(
        Node*                   node,
        UmtsLayer3Data*         umtsL3,
        UmtsSgsnUeDataInSgsn*   ueNas,
        UmtsLayer3SgsnFlowInfo* ueFlow,
        UmtsCallClearingType    clearType)
{
    UmtsCcNwData* ueCc = ueFlow->cmData.ccData;
    if (ueCc->state != UMTS_CC_NW_NULL
        && ueCc->state != UMTS_CC_NW_DISCONNECT_INDICATION
        && ueCc->state != UMTS_CC_NW_RELEASE_REQUEST)
    {
        UmtsLayer3MscStopCcTimers(node, ueCc);
        UmtsLayer3MscSendDisconnectToUe(node,
                                        umtsL3,
                                        ueNas,
                                        ueFlow,
                                        clearType);
        ueCc->state = UMTS_CC_NW_DISCONNECT_INDICATION;
        char timerInfo[8];
        memcpy(timerInfo, &(ueNas->ueId), sizeof(ueNas->ueId));
        int index = sizeof(ueNas->ueId);
        timerInfo[index++] = ueFlow->transId;
        timerInfo[index++] = (char)clearType;
        ueCc->T305 = UmtsLayer3SetTimer(
                            node,
                            umtsL3,
                            UMTS_LAYER3_TIMER_CC_T305,
                            UMTS_LAYER3_TIMER_CC_T305_VAL,
                            NULL,
                            (void*)&timerInfo[0],
                            index);
    }
}

// /**
// FUNCTION   :: UmtsLayer3MscSendCallSetupToGmsc
// LAYER      :: Layer 3
// PURPOSE    :: MSC sends a call setup to GMSC to
//               establish a mobile originating call.
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data*        : Pointer to the GSN Layer3 data
// + ueNas     : UmtsSgsnUeDataInSgsn*  : Pointer to the UE info data
// + ueFlow    : UmtsLayer3SgsnFlowInfo*: Pointer to the application flow
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3MscSendCallSetupToGmsc(
        Node*                   node,
        UmtsLayer3Data*         umtsL3,
        UmtsSgsnUeDataInSgsn*   ueNas,
        UmtsLayer3SgsnFlowInfo* ueFlow)
{
    UmtsLayer3Gsn* gsnL3 = (UmtsLayer3Gsn*) (umtsL3->dataPtr);
    UmtsCcNwData* ueCc = ueFlow->cmData.ccData;

    int pktSize = sizeof(ueCc->peerAddr) + sizeof(ueCc->callAppId)
                    +sizeof (ueCc->endTime) + sizeof(ueCc->avgTalkTime);
    Message* msg =
        MESSAGE_Alloc(node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        msg,
                        pktSize,
                        TRACE_UMTS_LAYER3);
    int index = 0;
    memcpy(MESSAGE_ReturnPacket(msg),
           &(ueCc->peerAddr),
           sizeof(ueCc->peerAddr));
    index += sizeof(ueCc->peerAddr);
    memcpy(MESSAGE_ReturnPacket(msg) + index,
           &(ueCc->callAppId),
           sizeof(ueCc->callAppId));
    index += sizeof(ueCc->callAppId);
    memcpy(MESSAGE_ReturnPacket(msg) + index,
           &(ueCc->endTime),
           sizeof(ueCc->endTime));
    index += sizeof(ueCc->endTime);
    memcpy(MESSAGE_ReturnPacket(msg) + index,
           &(ueCc->avgTalkTime),
           sizeof(ueCc->avgTalkTime));

    UmtsLayer3GsnSendCsSigMsgOverBackbone(
        node,
        msg,
        gsnL3->ggsnAddr,
        ueFlow->transId,
        ueNas->ueId,
        UMTS_CSSIG_MSG_CALL_SETUP);
}

// /**
// FUNCTION   :: UmtsLayer3MscSendAlertingToGmsc
// LAYER      :: Layer 3
// PURPOSE    :: MSC sends an ALERTING message to the GMSC.
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data*        : Pointer to the GSN Layer3 data
// + ueNas     : UmtsSgsnUeDataInSgsn*  : Pointer to the UE info data
// + ueFlow    : UmtsLayer3SgsnFlowInfo*: Pointer to the application flow
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3MscSendAlertingToGmsc(
        Node*                   node,
        UmtsLayer3Data*         umtsL3,
        UmtsSgsnUeDataInSgsn*   ueNas,
        UmtsLayer3SgsnFlowInfo* ueFlow)
{
    UmtsLayer3Gsn* gsnL3 = (UmtsLayer3Gsn*) (umtsL3->dataPtr);
    UmtsCcNwData* ueCc = ueFlow->cmData.ccData;

    Message* msg =
        MESSAGE_Alloc(node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node, msg, sizeof(NodeId), TRACE_UMTS_LAYER3);
    memcpy(MESSAGE_ReturnPacket(msg),
           &(ueCc->peerAddr),
           sizeof(ueCc->peerAddr));

    UmtsLayer3GsnSendCsSigMsgOverBackbone(
        node,
        msg,
        gsnL3->ggsnAddr,
        ueFlow->transId,
        ueNas->ueId,
        UMTS_CSSIG_MSG_CALL_ALERTING);
}

// /**
// FUNCTION   :: UmtsLayer3MscSendMobTermCallConnectToGmsc
// LAYER      :: Layer 3
// PURPOSE    :: MSC sends a CONNECT message to the GMSC.
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data*        : Pointer to the GSN Layer3 data
// + ueNas     : UmtsSgsnUeDataInSgsn*  : Pointer to the UE info data
// + ueFlow    : UmtsLayer3SgsnFlowInfo*: Pointer to the application flow
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3MscSendMobTermCallConnectToGmsc(
        Node*                   node,
        UmtsLayer3Data*         umtsL3,
        UmtsSgsnUeDataInSgsn*   ueNas,
        UmtsLayer3SgsnFlowInfo* ueFlow)
{
    UmtsLayer3Gsn* gsnL3 = (UmtsLayer3Gsn*) (umtsL3->dataPtr);

    Message* msg =
        MESSAGE_Alloc(node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node, msg, 0, TRACE_UMTS_LAYER3);

    UmtsLayer3GsnSendCsSigMsgOverBackbone(
        node,
        msg,
        gsnL3->ggsnAddr,
        ueFlow->transId,
        ueNas->ueId,
        UMTS_CSSIG_MSG_CALL_CONNECT);
}

// /**
// FUNCTION   :: UmtsLayer3MscSendDisconnectToGmsc
// LAYER      :: Layer 3
// PURPOSE    :: MSC sends a DISCONNECT message to the GMSC.
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data*        : Pointer to the GSN Layer3 data
// + ueId      : NodeId                 : Node Id of UE
// + ti        : char                   : transaction Id
// + clearType : UmtsCallClearType      : Call clear type
// + peerAddr  : NodeId                 : Peer address
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3MscSendDisconnectToGmsc(
        Node*                   node,
        UmtsLayer3Data*         umtsL3,
        NodeId                  ueId,
        char                    ti,
        UmtsCallClearingType    clearType,
        NodeId                  peerAddr)
{
    UmtsLayer3Gsn* gsnL3 = (UmtsLayer3Gsn*) (umtsL3->dataPtr);

    Message* msg =
        MESSAGE_Alloc(node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node,
                        msg,
                        sizeof(char) + sizeof(peerAddr),
                        TRACE_UMTS_LAYER3);
    *(msg->packet) = (char)clearType;
    memcpy(msg->packet+sizeof(char),
           &(peerAddr),
           sizeof(peerAddr));

    UmtsLayer3GsnSendCsSigMsgOverBackbone(
        node,
        msg,
        gsnL3->ggsnAddr,
        ti,
        ueId,
        UMTS_CSSIG_MSG_CALL_DISCONNECT);
}

// /**
// FUNCTION   :: UmtsLayer3GmscSendAlertingToMsc
// LAYER      :: Layer 3
// PURPOSE    :: GMSC sends an ALERTING to MSC
// PARAMETERS ::
// + node       : Node*                  : Pointer to node.
// + umtsL3     : UmtsLayer3Data*        : Pointer to the GSN Layer3 data
// + ueId       : NodeId                 : Node Id of UE
// + mobOrigcall : UmtsLayer3GmscCallInfo* : Info of the MO call
// RETURN       :: void : NULL
// **/
static
void UmtsLayer3GmscSendAlertingToMsc(
        Node*                   node,
        UmtsLayer3Data*         umtsL3,
        NodeId                  ueId,
        UmtsLayer3GmscCallInfo* mobOrigCall)
{
    Message* msg =
        MESSAGE_Alloc(node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node, msg, 0, TRACE_UMTS_LAYER3);

    UmtsLayer3GsnSendCsSigMsgOverBackbone(
        node,
        msg,
        mobOrigCall->mscAddr,
        mobOrigCall->ti,
        ueId,
        UMTS_CSSIG_MSG_CALL_ALERTING);
}

// /**
// FUNCTION   :: UmtsLayer3GmscSendConnectToMsc
// LAYER      :: Layer 3
// PURPOSE    :: GMSC sends a connect to MSC
// PARAMETERS ::
// + node       : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data*        : Pointer to the GSN Layer3 data
// + ueId       : NodeId                 : Node Id of UE
// + mobOrigcall : UmtsLayer3GmscCallInfo* : Info of the MO call
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3GmscSendConnectToMsc(
        Node*                   node,
        UmtsLayer3Data*         umtsL3,
        NodeId                  ueId,
        UmtsLayer3GmscCallInfo* mobOrigCall)
{
    Message* msg =
        MESSAGE_Alloc(node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node, msg, 0, TRACE_UMTS_LAYER3);

    UmtsLayer3GsnSendCsSigMsgOverBackbone(
        node,
        msg,
        mobOrigCall->mscAddr,
        mobOrigCall->ti,
        ueId,
        UMTS_CSSIG_MSG_CALL_CONNECT);
}

// /**
// FUNCTION   :: UmtsLayer3GmscSendDisconnectToMsc
// LAYER      :: Layer 3
// PURPOSE    :: GMSC sends a DISCONNECT to MSC
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data*        : Pointer to the GSN Layer3 data
// + ueId      : NodeId                 : Node Id of UE
// + clearingTermCall : UmtsLayer3GmscCallInfo* : info of the TO call
// + clearType : UmtsCallClearingType   : call clear type
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3GmscSendDisconnectToMsc(
        Node*                   node,
        UmtsLayer3Data*         umtsL3,
        NodeId                  ueId,
        UmtsLayer3GmscCallInfo* clearingTermCall,
        UmtsCallClearingType    clearType)
{
    char msgBuf[12];
    int index = 0;

    memcpy(&msgBuf[index],
           &(clearingTermCall->peerAddr),
           sizeof(clearingTermCall->peerAddr));
    index += sizeof(clearingTermCall->peerAddr);

    memcpy(&msgBuf[index],
           &(clearingTermCall->callAppId),
           sizeof(clearingTermCall->callAppId));
    index += sizeof(clearingTermCall->callAppId);
    msgBuf[index++] = (char)clearType;

    Message* msg =
        MESSAGE_Alloc(node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(node, msg, index, TRACE_UMTS_LAYER3);
    memcpy(MESSAGE_ReturnPacket(msg),
           msgBuf,
           index);
    UmtsLayer3GsnSendCsSigMsgOverBackbone(
        node,
        msg,
        clearingTermCall->mscAddr,
        clearingTermCall->ti,
        ueId,
        UMTS_CSSIG_MSG_CALL_DISCONNECT);
}

// /**
// FUNCTION   :: UmtsLayer3GmscSendCallSetupToMsc
// LAYER      :: Layer 3
// PURPOSE    :: GMSC sends a call setup MSC to
//               set up a mobile terminating call
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data*        : Pointer to the GSN Layer3 data
// + ueId      : NodeId                 : Node Id of UE
// + mobTermCall : UmtsLayer3GmscCallInfo* : Info of the MT call
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3GmscSendCallSetupToMsc(
    Node*                   node,
    UmtsLayer3Data*         umtsL3,
    NodeId                  ueId,
    UmtsLayer3GmscCallInfo* mobTermCall)
{
    int pktSize = sizeof(mobTermCall->peerAddr)
                    + sizeof(mobTermCall->callAppId)
                    + sizeof(mobTermCall->endTime)
                    + sizeof(mobTermCall->avgTalkTime);
    int index = 0;
    Message* msg =
        MESSAGE_Alloc(node, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR, 0);
    MESSAGE_PacketAlloc(
        node,
        msg,
        pktSize,
        TRACE_UMTS_LAYER3);
    memcpy(MESSAGE_ReturnPacket(msg),
           &(mobTermCall->peerAddr),
           sizeof(mobTermCall->peerAddr));
    index += sizeof(mobTermCall->peerAddr);
    memcpy(MESSAGE_ReturnPacket(msg) + index,
           &(mobTermCall->callAppId),
           sizeof(mobTermCall->callAppId));
    index += sizeof(mobTermCall->callAppId);
    memcpy(MESSAGE_ReturnPacket(msg) + index,
           &(mobTermCall->endTime),
           sizeof(mobTermCall->endTime));
    index += sizeof(mobTermCall->endTime);
    memcpy(MESSAGE_ReturnPacket(msg) + index,
           &(mobTermCall->avgTalkTime),
           sizeof(mobTermCall->avgTalkTime));

    UmtsLayer3GsnSendCsSigMsgOverBackbone(
        node,
        msg,
        mobTermCall->mscAddr,
        0,
        ueId,
        UMTS_CSSIG_MSG_CALL_SETUP);
}

// /**
// FUNCTION   :: UmtsLayer3MscHandleMobOrigCallSetup
// LAYER      :: Layer 3
// PURPOSE    :: Handle a CC SETUP message
// PARAMETERS ::
// + node      : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data*        : Pointer to the GSN Layer3 data
// + ueNas     : UmtsSgsnUeDataInSgsn*  : Pointer to the UE info data
// + transactId     : char                   : Transaction ID
// + msg            : char*   : Pointer to the NAS message contents
// + msgSize        : int                    : Message size
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3MscHandleMobOrigCallSetup(
    Node*                   node,
    UmtsLayer3Data*         umtsL3,
    UmtsSgsnUeDataInSgsn*   ueNas,
    unsigned char           ti,
    char*                   msg,
    int                     msgSize)
{
    UmtsLayer3Gsn* gsnL3 = (UmtsLayer3Gsn*) (umtsL3->dataPtr);
    gsnL3->sgsnStat.numCcSetupRcvd++;

    if (DEBUG_CC)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_CC);
        printf("\n\t MSC rcvd a mobile originating "
            "call SETUP from UE: %d, TI: %d\n",
            ueNas->ueId, ti);
    }
    UmtsLayer3SgsnFlowInfo* ueFlow = UmtsLayer3GsnFindAppInfoFromList(
                                        node,
                                        gsnL3,
                                        ueNas,
                                        ti);

    if (!ueFlow)
    {
        UmtsLayer3FlowClassifier appClsInfo;
        appClsInfo.domainInfo = UMTS_LAYER3_CN_DOMAIN_CS;
        ueFlow = UmtsLayer3GsnAddAppInfoToList(
                        node,
                        gsnL3,
                        ueNas->appInfoList,
                        &appClsInfo,
                        UMTS_FLOW_MOBILE_ORIGINATED,
                        ti);
        ueFlow->ueId = ueNas->ueId;

        int index = 0;
        UmtsCcNwData* ueCc = ueFlow->cmData.ccData;
        memcpy(&ueCc->peerAddr, msg, sizeof(ueCc->peerAddr));
        index += sizeof(ueCc->peerAddr);
        memcpy(&ueCc->callAppId, msg + index, sizeof(ueCc->callAppId));
        index += sizeof(ueCc->callAppId);
        memcpy(&ueCc->endTime, msg + index, sizeof(ueCc->endTime));
        index += sizeof(ueCc->endTime);
        memcpy(&ueCc->avgTalkTime, msg + index, sizeof(ueCc->avgTalkTime));
        index += sizeof(ueCc->avgTalkTime);

        //signalling GMSC to process call
        UmtsLayer3MscSendCallSetupToGmsc(node, umtsL3, ueNas, ueFlow);

        //request to set RAB
        UmtsLayer3MscSendVoiceCallRabAssgt(node, umtsL3, ueNas->ueId, ti);

        UmtsLayer3MscSendCallProcToUe(node, umtsL3, ueNas, ueFlow);
        ueCc->state = UMTS_CC_NW_MOBILE_ORIGINATING_CALL_PROCEEDING;
    }
}

// FUNCTION   :: UmtsLayer3MscHandleCallConnAck
// LAYER      :: Layer 3
// PURPOSE    :: Handle a CC CONNECT ACKNOWLEDGE message
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data*        : Pointer to the GSN Layer3 data
// + ueNas          : UmtsSgsnUeDataInSgsn*  : Pointer to the UE info data
// + transactId     : char                   : Transaction ID
// + msg            : char*   : Pointer to the NAS message contents
// + msgSize        : int                    : Message size
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3MscHandleCallConnAck(
    Node*                   node,
    UmtsLayer3Data*         umtsL3,
    UmtsSgsnUeDataInSgsn*   ueNas,
    unsigned char           ti,
    char*                   msg,
    int                     msgSize)
{
    if (DEBUG_CC)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_CC);
        printf("\n\t MSC rcvd a CONNECT ACK from UE: %d, TI: %d\n",
            ueNas->ueId, ti);
    }
    UmtsLayer3Gsn* gsnL3 = (UmtsLayer3Gsn*)(umtsL3->dataPtr);
    gsnL3->sgsnStat.numCcConnAckRcvd++;

    UmtsLayer3SgsnFlowInfo* ueFlow = UmtsLayer3GsnFindAppInfoFromList(
                                        node,
                                        gsnL3,
                                        ueNas,
                                        ti);
    if (ueFlow)
    {
        UmtsCcNwData* ueCc = ueFlow->cmData.ccData;
        if (ueCc->state == UMTS_CC_NW_CONNECT_INDICATION)
        {
            ERROR_Assert(ueCc->T313, "inconsistent UE CC state.");
            MESSAGE_CancelSelfMsg(node, ueCc->T313);
            ueCc->T313 = NULL;
            ueCc->state = UMTS_CC_NW_ACTIVE;
        }
    }
}

// FUNCTION   :: UmtsLayer3MscHandleCallConfirm
// LAYER      :: Layer 3
// PURPOSE    :: Handle a CC CALL CONFIRMED message
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3    : UmtsLayer3Data*        : Pointer to the GSN Layer3 data
// + ueNas          : UmtsSgsnUeDataInSgsn*  : Pointer to the UE info data
// + transactId     : char                   : Transaction ID
// + msg            : char*      : Pointer to the NAS message contents
// + msgSize        : int                    : Message size
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3MscHandleCallConfirm(
    Node*                   node,
    UmtsLayer3Data*         umtsL3,
    UmtsSgsnUeDataInSgsn*   ueNas,
    unsigned char           ti,
    char*                   msg,
    int                     msgSize)
{
    if (DEBUG_CC)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_CC);
        printf("\n\t MSC rcvd a CALL CONFIRM from UE: %d, TI: %d\n",
            ueNas->ueId, ti);
    }
    UmtsLayer3Gsn* gsnL3 = (UmtsLayer3Gsn*)(umtsL3->dataPtr);
    gsnL3->sgsnStat.numCcCallConfirmRcvd++;

    UmtsLayer3SgsnFlowInfo* ueFlow = UmtsLayer3GsnFindAppInfoFromList(
                                        node,
                                        gsnL3,
                                        ueNas,
                                        ti);
    if (ueFlow)
    {
        UmtsCcNwData* ueCc = ueFlow->cmData.ccData;
        if (ueCc->state == UMTS_CC_NW_CALL_PRESENT)
        {
            if (ueCc->T303)
            {
                MESSAGE_CancelSelfMsg(node, ueCc->T303);
                ueCc->T303 = NULL;
            }
            ueCc->state = UMTS_CC_NW_MOBILE_TERMINATING_CALL_CONFIRMED;

            //Initiate timer T310
            char timerInfo[5];
            memcpy(timerInfo, &(ueNas->ueId), sizeof(ueNas->ueId));
            int index = sizeof(ueNas->ueId);
            timerInfo[index++] = ueFlow->transId;
            ueCc->T310 = UmtsLayer3SetTimer(
                                node,
                                umtsL3,
                                UMTS_LAYER3_TIMER_CC_T310,
                                UMTS_LAYER3_TIMER_CC_T310_NW_VAL,
                                NULL,
                                (void*)&timerInfo[0],
                                index);
        }
    }
}

// FUNCTION   :: UmtsLayer3MscHandleAlertingFromUe
// LAYER      :: Layer 3
// PURPOSE    :: Handle an ALERTING message from the UE
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data*  : Pointer to the GSN Layer3 data
// + ueNas          : UmtsSgsnUeDataInSgsn*  : Pointer to the UE info data
// + ti             : char                   : Transaction ID
// + msg            : char*   : Pointer to the NAS message contents
// + msgSize        : int                    : Message size
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3MscHandleAlertingFromUe(
    Node*                   node,
    UmtsLayer3Data*         umtsL3,
    UmtsSgsnUeDataInSgsn*   ueNas,
    unsigned char           ti,
    char*                   msg,
    int                     msgSize)
{
    if (DEBUG_CC)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_CC);
        printf("\n\t MSC rcvd an ALERTING from UE: %d, TI: %d\n",
            ueNas->ueId, ti);
    }
    UmtsLayer3Gsn* gsnL3 = (UmtsLayer3Gsn*)(umtsL3->dataPtr);
    gsnL3->sgsnStat.numCcAlertingRcvd++;

    UmtsLayer3SgsnFlowInfo* ueFlow = UmtsLayer3GsnFindAppInfoFromList(
                                        node,
                                        gsnL3,
                                        ueNas,
                                        ti);
    if (ueFlow)
    {
        UmtsCcNwData* ueCc = ueFlow->cmData.ccData;

        if (ueCc->state == UMTS_CC_NW_CALL_PRESENT ||
            ueCc->state == UMTS_CC_NW_MOBILE_TERMINATING_CALL_CONFIRMED)
        {
            if (ueCc->T310)
            {
                MESSAGE_CancelSelfMsg(node, ueCc->T310);
                ueCc->T310 = NULL;
            }
            ueCc->state = UMTS_CC_NW_CALL_RECEIVED;

            //Initiate timer T301
            char timerInfo[5];
            memcpy(timerInfo, &(ueNas->ueId), sizeof(ueNas->ueId));
            int index = sizeof(ueNas->ueId);
            timerInfo[index++] = ueFlow->transId;
            ueCc->T301 = UmtsLayer3SetTimer(
                                node,
                                umtsL3,
                                UMTS_LAYER3_TIMER_CC_T301,
                                UMTS_LAYER3_TIMER_CC_T301_VAL,
                                NULL,
                                (void*)&timerInfo[0],
                                index);

            //Send Alerting to the calling part
            UmtsLayer3MscSendAlertingToGmsc(node,
                                            umtsL3,
                                            ueNas,
                                            ueFlow);
        }
    }
}

// FUNCTION   :: UmtsLayer3MscHandleConnectFromUe
// LAYER      :: Layer 3
// PURPOSE    :: Handle a CONNECT message from the UE
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data*  : Pointer to the GSN Layer3 data
// + ueNas          : UmtsSgsnUeDataInSgsn*  : Pointer to the UE info data
// + ti             : char                   : Transaction ID
// + msg            : char*         : Pointer to the NAS message contents
// + msgSize        : int                    : Message size
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3MscHandleConnectFromUe(
    Node*                   node,
    UmtsLayer3Data*         umtsL3,
    UmtsSgsnUeDataInSgsn*   ueNas,
    unsigned char           ti,
    char*                   msg,
    int                     msgSize)
{
    if (DEBUG_CC)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_CC);
        printf("\n\t MSC rcvd a CONNECT from UE: %d, TI: %d\n",
            ueNas->ueId, ti);
    }
    UmtsLayer3Gsn* gsnL3 = (UmtsLayer3Gsn*)(umtsL3->dataPtr);
    gsnL3->sgsnStat.numCcConnectRcvd++;

    UmtsLayer3SgsnFlowInfo* ueFlow = UmtsLayer3GsnFindAppInfoFromList(
                                        node,
                                        gsnL3,
                                        ueNas,
                                        ti);
    //If RAB has been assigned for this phone call
    if (ueFlow)
    {
        UmtsCcNwData* ueCc = ueFlow->cmData.ccData;
        if (ueCc->state == UMTS_CC_NW_CALL_PRESENT ||
            ueCc->state == UMTS_CC_NW_MOBILE_TERMINATING_CALL_CONFIRMED ||
            ueCc->state == UMTS_CC_NW_CALL_RECEIVED)
        {
            if (ueCc->T301)
            {
                MESSAGE_CancelSelfMsg(node, ueCc->T301);
                ueCc->T301 = NULL;
            }
            if (ueCc->T310)
            {
                MESSAGE_CancelSelfMsg(node, ueCc->T301);
                ueCc->T310 = NULL;
            }
            if (ueCc->T303)
            {
                MESSAGE_CancelSelfMsg(node, ueCc->T303);
                ueCc->T303 = NULL;
            }

            if (ueFlow->rabId != UMTS_INVALID_RAB_ID)
            {
                UmtsLayer3MscSendConnAckToUe(node, umtsL3, ueNas, ueFlow);
                UmtsLayer3MscSendMobTermCallConnectToGmsc(
                    node,
                    umtsL3,
                    ueNas,
                    ueFlow);
                ueCc->state = UMTS_CC_NW_ACTIVE;
            }
            else
            {
                ueCc->state = UMTS_CC_NW_ACTIVE_PENDING_ON_RAB_EST;
            }
        }
    }
}

// FUNCTION   :: UmtsLayer3MscHandleDisconnectFromUe
// LAYER      :: Layer 3
// PURPOSE    :: Handle a DISCONNECT message from the UE
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data*   : Pointer to the GSN Layer3 data
// + ueNas          : UmtsSgsnUeDataInSgsn*  : Pointer to the UE info data
// + ti             : char                   : Transaction ID
// + msg            : char*      : Pointer to the NAS message contents
// + msgSize        : int                    : Message size
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3MscHandleDisconnectFromUe(
    Node*                   node,
    UmtsLayer3Data*         umtsL3,
    UmtsSgsnUeDataInSgsn*   ueNas,
    unsigned char           ti,
    char*                   msg,
    int                     msgSize)
{
    if (DEBUG_CC)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_CC);
        printf("\n\t MSC rcvd a DISCONNECT from UE: %d, TI: %d\n",
            ueNas->ueId, ti);
    }
    UmtsLayer3Gsn* gsnL3 = (UmtsLayer3Gsn*)(umtsL3->dataPtr);
    gsnL3->sgsnStat.numCcDisconnectRcvd++;

    UmtsLayer3SgsnFlowInfo* ueFlow = UmtsLayer3GsnFindAppInfoFromList(
                                        node,
                                        gsnL3,
                                        ueNas,
                                        ti);
    if (ueFlow)
    {
        UmtsCcNwData* ueCc = ueFlow->cmData.ccData;
        if (ueCc->state != UMTS_CC_NW_NULL &&
            ueCc->state != UMTS_CC_NW_RELEASE_REQUEST)
        {
            UmtsLayer3MscStopCcTimers(node, ueCc);

            UmtsCallClearingType clearType = (UmtsCallClearingType)(*msg);
            //Initiate procedure to clear the network connection and
            //the call to the remote user
            UmtsLayer3MscSendDisconnectToGmsc(node,
                                              umtsL3,
                                              ueNas->ueId,
                                              ueFlow->transId,
                                              clearType,
                                              ueCc->peerAddr);

            UmtsLayer3MscSendReleaseToUe(node, umtsL3, ueNas, ueFlow);
            char timerInfo[5];
            memcpy(timerInfo, &(ueNas->ueId), sizeof(ueNas->ueId));
            int index = sizeof(ueNas->ueId);
            timerInfo[index++] = ueFlow->transId;
            ueCc->T308 = UmtsLayer3SetTimer(
                                node,
                                umtsL3,
                                UMTS_LAYER3_TIMER_CC_T308,
                                UMTS_LAYER3_TIMER_CC_T308_NW_VAL,
                                NULL,
                                (void*)&timerInfo[0],
                                index);
            ueCc->state = UMTS_CC_NW_RELEASE_REQUEST;
            ueCc->counterT308 = 0;
        }
    }
}

// FUNCTION   :: UmtsLayer3MscHandleReleaseFromUe
// LAYER      :: Layer 3
// PURPOSE    :: Handle a RELEASE message from the UE
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data* : Pointer to the GSN Layer3 data
// + ueNas          : UmtsSgsnUeDataInSgsn*  : Pointer to the UE info data
// + ti             : char                   : Transaction ID
// + msg            : char*      : Pointer to the NAS message contents
// + msgSize        : int                    : Message size
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3MscHandleReleaseFromUe(
    Node*                   node,
    UmtsLayer3Data*         umtsL3,
    UmtsSgsnUeDataInSgsn*   ueNas,
    unsigned char           ti,
    char*                   msg,
    int                     msgSize)
{
    if (DEBUG_CC)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_CC);
        printf("\n\t MSC rcvd a RELEASE from UE: %d, TI: %d\n",
            ueNas->ueId, ti);
    }
    UmtsLayer3Gsn* gsnL3 = (UmtsLayer3Gsn*)(umtsL3->dataPtr);
    gsnL3->sgsnStat.numCcReleaseRcvd++;

    UmtsLayer3SgsnFlowInfo* ueFlow = UmtsLayer3GsnFindAppInfoFromList(
                                        node,
                                        gsnL3,
                                        ueNas,
                                        ti);
    if (ueFlow && ueFlow->cmData.ccData->state != UMTS_CC_NW_NULL
        && ueFlow->cmData.ccData->state!= UMTS_CC_NW_RELEASE_REQUEST)
    {
        UmtsCcNwData* ueCc = ueFlow->cmData.ccData;
        UmtsLayer3MscStopCcTimers(node, ueCc);
        if (ueCc->T305)
        {
            MESSAGE_CancelSelfMsg(node, ueCc->T305);
            ueCc->T305 = NULL;
        }

        //If the network CC entity hasn't sent DISCONNEC to the UE,
        //it needs to report the network that the UE decides to
        //drop the call.
        if (ueCc->state != UMTS_CC_NW_DISCONNECT_INDICATION)
        {
            UmtsLayer3MscSendDisconnectToGmsc(node,
                                              umtsL3,
                                              ueNas->ueId,
                                              ueFlow->transId,
                                              UMTS_CALL_CLEAR_DROP,
                                              ueCc->peerAddr);
        }
        UmtsLayer3MscSendReleaseCompleteToUe(node, umtsL3, ueNas, ueFlow);

        //release RAB if allocated
        if (ueFlow->rabId != UMTS_INVALID_RAB_ID)
        {
            UmtsLayer3MscSendVoiceCallRabRelease(node,
                                                 umtsL3,
                                                 ueNas->ueId,
                                                 ueFlow->rabId);
        }

        ueCc->state = UMTS_CC_NW_NULL;
        char ti = ueFlow->transId;
        UmtsLayer3GsnRemoveAppInfoFromList(
            node,
            gsnL3,
            ueNas->transIdBitSet,
            ueNas->appInfoList,
            ueFlow->srcDestType,
            ueFlow->transId);
        UmtsLayer3GsnReleaseMmConn(
            node,
            umtsL3,
            ueNas,
            UMTS_LAYER3_PD_CC,
            ti);
   }
}

// FUNCTION   :: UmtsLayer3MscHandleReleaseComplete
// LAYER      :: Layer 3
// PURPOSE    :: Handle a RELEASE COMPLETE message from the UE
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data*  : Pointer to the GSN Layer3 data
// + ueNas          : UmtsSgsnUeDataInSgsn*  : Pointer to the UE info data
// + ti             : char                   : Transaction ID
// + msg            : char*          : Pointer to the NAS message contents
// + msgSize        : int                    : Message size
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3MscHandleReleaseComplete(
    Node*                   node,
    UmtsLayer3Data*         umtsL3,
    UmtsSgsnUeDataInSgsn*   ueNas,
    unsigned char           ti,
    char*                   msg,
    int                     msgSize)
{
    if (DEBUG_CC)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_CC);
        printf("\n\t MSC rcvd a RELEASE COMPLETE from UE: %d, TI: %d\n",
            ueNas->ueId, ti);
    }
    UmtsLayer3Gsn* gsnL3 = (UmtsLayer3Gsn*)(umtsL3->dataPtr);
    gsnL3->sgsnStat.numCcReleaseCompleteRcvd++;

    UmtsLayer3SgsnFlowInfo* ueFlow = UmtsLayer3GsnFindAppInfoFromList(
                                        node,
                                        gsnL3,
                                        ueNas,
                                        ti);
    if (ueFlow)
    {
        UmtsCcNwData* ueCc = ueFlow->cmData.ccData;
        UmtsLayer3MscStopCcTimers(node, ueCc);
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

        //release RAB if allocated
        if (ueFlow->rabId != UMTS_INVALID_RAB_ID)
        {
            UmtsLayer3MscSendVoiceCallRabRelease(node,
                                                 umtsL3,
                                                 ueNas->ueId,
                                                 ueFlow->rabId);
        }
        char ti = ueFlow->transId;
        UmtsLayer3GsnRemoveAppInfoFromList(
            node,
            gsnL3,
            ueNas->transIdBitSet,
            ueNas->appInfoList,
            ueFlow->srcDestType,
            ueFlow->transId);
        UmtsLayer3GsnReleaseMmConn(
            node,
            umtsL3,
            ueNas,
            UMTS_LAYER3_PD_CC,
            ti);
    }
}

// /**
// FUNCTION   :: UmtsLayer3MsgHandleCcMsg
// LAYER      :: Layer 3
// PURPOSE    :: Handle a CC message from a UE .
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data*   : Pointer to the GSN Layer3 data
// + ueNas          : UmtsSgsnUeDataInSgsn*  : Pointer to the UE info data
// + msgType        : UmtsCcMessageType      : Message type
// + transactId     : char                   : Transaction ID
// + msg            : char*          : Pointer to the NAS message contents
// + msgSize        : int                    : Message size
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3MscHandleCcMsg(
        Node*                   node,
        UmtsLayer3Data*         umtsL3,
        UmtsSgsnUeDataInSgsn*   ueNas,
        UmtsCcMessageType       msgType,
        unsigned char           ti,
        char*                   msg,
        int                     msgSize)
{
    switch (msgType)
    {
        case UMTS_CC_MSG_TYPE_SETUP:
        {
            UmtsLayer3MscHandleMobOrigCallSetup(node,
                                                umtsL3,
                                                ueNas,
                                                ti,
                                                msg,
                                                msgSize);
            break;
        }
        case UMTS_CC_MSG_TYPE_CONNECT_ACKNOWLEDGE:
        {
            UmtsLayer3MscHandleCallConnAck(node,
                                           umtsL3,
                                           ueNas,
                                           ti,
                                           msg,
                                           msgSize);
            break;
        }
        case UMTS_CC_MSG_TYPE_CALL_CONFIRMED:
        {
            UmtsLayer3MscHandleCallConfirm(node,
                                           umtsL3,
                                           ueNas,
                                           ti,
                                           msg,
                                           msgSize);
            break;
        }
        case UMTS_CC_MSG_TYPE_ALERTING:
        {
            UmtsLayer3MscHandleAlertingFromUe(node,
                                              umtsL3,
                                              ueNas,
                                              ti,
                                              msg,
                                              msgSize);
            break;
        }
        case UMTS_CC_MSG_TYPE_CONNECT:
        {
            UmtsLayer3MscHandleConnectFromUe(node,
                                             umtsL3,
                                             ueNas,
                                             ti,
                                             msg,
                                             msgSize);
            break;
        }
        case UMTS_CC_MSG_TYPE_DISCONNECT:
        {
            UmtsLayer3MscHandleDisconnectFromUe(
                node,
                umtsL3,
                ueNas,
                ti,
                msg,
                msgSize);
            break;
        }
        case UMTS_CC_MSG_TYPE_RELEASE:
        {
            UmtsLayer3MscHandleReleaseFromUe(node,
                                             umtsL3,
                                             ueNas,
                                             ti,
                                             msg,
                                             msgSize);
            break;
        }
        case UMTS_CC_MSG_TYPE_RELEASE_COMPLETE:
        {
            UmtsLayer3MscHandleReleaseComplete(node,
                                               umtsL3,
                                               ueNas,
                                               ti,
                                               msg,
                                               msgSize);
            break;
        }
        default:
            ERROR_ReportError("Received an unidentified CC message.");
    }
}

// /**
// FUNCTION   :: UmtsLayer3MscHandleT301
// LAYER      :: Layer 3
// PURPOSE    :: Handle a CC T301  timer expired msg
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + gsnL3     : UmtsLayer3Gsn *  : UE data
// + msg       : Message*         : Message containing the packet
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3MscHandleT301(
        Node *node,
        UmtsLayer3Data *umtsL3,
        UmtsLayer3Gsn *gsnL3,
        Message *msg)
{
    char* timerInfo = MESSAGE_ReturnInfo(msg) + sizeof(UmtsLayer3Timer);
    NodeId ueId;
    char  ti;

    int index = 0;
    memcpy(&ueId, &timerInfo[index], sizeof(ueId));
    index += sizeof(ueId);
    ti = timerInfo[index ++];

    std::list<UmtsSgsnUeDataInSgsn*>::iterator ueNasIt;
    ueNasIt = std::find_if(
                gsnL3->ueInfoList->begin(),
                gsnL3->ueInfoList->end(),
                UmtsFindItemByUeId<UmtsSgsnUeDataInSgsn>(ueId));

    if (ueNasIt != gsnL3->ueInfoList->end())
    {
        UmtsLayer3SgsnFlowInfo* flowInfo =
            UmtsLayer3GsnFindAppInfoFromList(
                    node,
                    gsnL3,
                    (*ueNasIt),
                    ti);
        if (flowInfo)
        {
            UmtsCcNwData* ueCc = flowInfo->cmData.ccData;
            if (ueCc->state == UMTS_CC_NW_CALL_RECEIVED)
            {
                //start to clear the call
                UmtsLayer3MscInitCallClearing(
                    node,
                    umtsL3,
                    (*ueNasIt),
                    flowInfo,
                    UMTS_CALL_CLEAR_DROP);
                //start to clear the call toward the GMSC
                UmtsLayer3MscSendDisconnectToGmsc(
                    node,
                    umtsL3,
                    (*ueNasIt)->ueId,
                    flowInfo->transId,
                    UMTS_CALL_CLEAR_REJECT,
                    ueCc->peerAddr);
            } // if (ueCc->state)
        } //  if (flowInfo)
    } // if (ueNasIt)
}

// /**
// FUNCTION   :: UmtsLayer3MscHandleT303
// LAYER      :: Layer 3
// PURPOSE    :: Handle a CC T303 timer expired msg
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + gsnL3     : UmtsLayer3Gsn *  : UE data
// + msg       : Message*         : Message containing the packet
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3MscHandleT303(
        Node *node,
        UmtsLayer3Data *umtsL3,
        UmtsLayer3Gsn *gsnL3,
        Message *msg)
{
    char* timerInfo = MESSAGE_ReturnInfo(msg) + sizeof(UmtsLayer3Timer);
    NodeId ueId;
    char  ti;

    int index = 0;
    memcpy(&ueId, &timerInfo[index], sizeof(ueId));
    index += sizeof(ueId);
    ti = timerInfo[index ++];

    std::list<UmtsSgsnUeDataInSgsn*>::iterator ueNasIt;
    ueNasIt = std::find_if(
                gsnL3->ueInfoList->begin(),
                gsnL3->ueInfoList->end(),
                UmtsFindItemByUeId<UmtsSgsnUeDataInSgsn>(ueId));

    if (ueNasIt != gsnL3->ueInfoList->end())
    {
        UmtsLayer3SgsnFlowInfo* flowInfo =
            UmtsLayer3GsnFindAppInfoFromList(
                    node,
                    gsnL3,
                    (*ueNasIt),
                    ti);
        if (flowInfo)
        {
            UmtsCcNwData* ueCc = flowInfo->cmData.ccData;
            ueCc->T303 = NULL;
            if (ueCc->state == UMTS_CC_NW_CALL_PRESENT)
            {
                //start to clear the call
                UmtsLayer3MscInitCallClearing(
                    node,
                    umtsL3,
                    (*ueNasIt),
                    flowInfo,
                    UMTS_CALL_CLEAR_DROP);
                //start to clear the call toward the GMSC
                UmtsLayer3MscSendDisconnectToGmsc(
                    node,
                    umtsL3,
                    (*ueNasIt)->ueId,
                    flowInfo->transId,
                    UMTS_CALL_CLEAR_REJECT,
                    ueCc->peerAddr);
            } // if (ueCc->state
        } //  if (flowInfo)
    } // if (ueNasIt)
}

// /**
// FUNCTION   :: UmtsLayer3MscHandleT310
// LAYER      :: Layer 3
// PURPOSE    :: Handle a CC T310 timer expired msg
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + gsnL3     : UmtsLayer3Gsn *  : UE data
// + msg       : Message*         : Message containing the packet
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3MscHandleT310(
        Node *node,
        UmtsLayer3Data *umtsL3,
        UmtsLayer3Gsn *gsnL3,
        Message *msg)
{
    char* timerInfo = MESSAGE_ReturnInfo(msg) + sizeof(UmtsLayer3Timer);
    NodeId ueId;
    char  ti;

    int index = 0;
    memcpy(&ueId, &timerInfo[index], sizeof(ueId));
    index += sizeof(ueId);
    ti = timerInfo[index ++];

    std::list<UmtsSgsnUeDataInSgsn*>::iterator ueNasIt;
    ueNasIt = std::find_if(
                gsnL3->ueInfoList->begin(),
                gsnL3->ueInfoList->end(),
                UmtsFindItemByUeId<UmtsSgsnUeDataInSgsn>(ueId));

    if (ueNasIt != gsnL3->ueInfoList->end())
    {
        UmtsLayer3SgsnFlowInfo* flowInfo =
            UmtsLayer3GsnFindAppInfoFromList(
                    node,
                    gsnL3,
                    (*ueNasIt),
                    ti);
        if (flowInfo)
        {
            UmtsCcNwData* ueCc = flowInfo->cmData.ccData;
            ueCc->T310 = NULL;
            if (ueCc->state == UMTS_CC_NW_MOBILE_TERMINATING_CALL_CONFIRMED)
            {
                //start to clear the call
                UmtsLayer3MscInitCallClearing(
                    node,
                    umtsL3,
                    (*ueNasIt),
                    flowInfo,
                    UMTS_CALL_CLEAR_DROP);
                //start to clear the call toward the GMSC
                UmtsLayer3MscSendDisconnectToGmsc(
                    node,
                    umtsL3,
                    (*ueNasIt)->ueId,
                    flowInfo->transId,
                    UMTS_CALL_CLEAR_REJECT,
                    ueCc->peerAddr);
            } // if (ueCc->state)
        } // if (flowInfo)
    } // if (ueNasIt)
}

// /**
// FUNCTION   :: UmtsLayer3MscHandleT313
// LAYER      :: Layer 3
// PURPOSE    :: Handle a CC T313 timer expired msg
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + gsnL3     : UmtsLayer3Gsn *  : UE data
// + msg       : Message*         : Message containing the packet
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3MscHandleT313(
        Node *node,
        UmtsLayer3Data *umtsL3,
        UmtsLayer3Gsn *gsnL3,
        Message *msg)
{
    char* timerInfo = MESSAGE_ReturnInfo(msg) + sizeof(UmtsLayer3Timer);
    NodeId ueId;
    char  ti;

    int index = 0;
    memcpy(&ueId, &timerInfo[index], sizeof(ueId));
    index += sizeof(ueId);
    ti = timerInfo[index ++];

    std::list<UmtsSgsnUeDataInSgsn*>::iterator ueNasIt;
    ueNasIt = std::find_if(
                gsnL3->ueInfoList->begin(),
                gsnL3->ueInfoList->end(),
                UmtsFindItemByUeId<UmtsSgsnUeDataInSgsn>(ueId));

    if (ueNasIt != gsnL3->ueInfoList->end())
    {
        UmtsLayer3SgsnFlowInfo* flowInfo =
            UmtsLayer3GsnFindAppInfoFromList(
                    node,
                    gsnL3,
                    (*ueNasIt),
                    ti);
        if (flowInfo)
        {
            UmtsCcNwData* ueCc = flowInfo->cmData.ccData;
            ueCc->T313 = NULL;
            if (ueCc->state == UMTS_CC_NW_CONNECT_INDICATION)
            {
                //start to clear the call
                UmtsLayer3MscInitCallClearing(
                    node,
                    umtsL3,
                    (*ueNasIt),
                    flowInfo,
                    UMTS_CALL_CLEAR_DROP);
                //start to clear the call toward the GMSC
                UmtsLayer3MscSendDisconnectToGmsc(
                    node,
                    umtsL3,
                    (*ueNasIt)->ueId,
                    flowInfo->transId,
                    UMTS_CALL_CLEAR_DROP,
                    ueCc->peerAddr);
            }
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3MscHandleT305
// LAYER      :: Layer 3
// PURPOSE    :: Handle a CC T305 timer expired msg
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + gsnL3     : UmtsLayer3Gsn *  : UE data
// + msg       : Message*         : Message containing the packet
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3MscHandleT305(
        Node *node,
        UmtsLayer3Data *umtsL3,
        UmtsLayer3Gsn *gsnL3,
        Message *msg)
{
    char* timerInfo = MESSAGE_ReturnInfo(msg) + sizeof(UmtsLayer3Timer);
    NodeId ueId;
    char  ti;
    UmtsCallClearingType clearType;

    int index = 0;
    memcpy(&ueId, &timerInfo[index], sizeof(ueId));
    index += sizeof(ueId);
    ti = timerInfo[index ++];
    clearType = (UmtsCallClearingType)(timerInfo[index++]);

    std::list<UmtsSgsnUeDataInSgsn*>::iterator ueNasIt;
    ueNasIt = std::find_if(
                gsnL3->ueInfoList->begin(),
                gsnL3->ueInfoList->end(),
                UmtsFindItemByUeId<UmtsSgsnUeDataInSgsn>(ueId));

    if (ueNasIt != gsnL3->ueInfoList->end())
    {
        UmtsSgsnUeDataInSgsn* ueNas = *ueNasIt;
        UmtsLayer3SgsnFlowInfo* flowInfo =
            UmtsLayer3GsnFindAppInfoFromList(
                    node,
                    gsnL3,
                    (*ueNasIt),
                    ti);
        if (flowInfo)
        {
            UmtsCcNwData* ueCc = flowInfo->cmData.ccData;
            ueCc->T305 = NULL;
            if (ueCc->state == UMTS_CC_NW_DISCONNECT_INDICATION)
            {
                UmtsLayer3MscSendReleaseToUe(node,
                                             umtsL3,
                                             (*ueNasIt),
                                             flowInfo,
                                             clearType);
                char timerInfo[6];
                memcpy(timerInfo, &(ueNas->ueId), sizeof(ueNas->ueId));
                int index = sizeof(ueNas->ueId);
                timerInfo[index++] = flowInfo->transId;
                timerInfo[index++] = (char)clearType;
                ueCc->T308 = UmtsLayer3SetTimer(
                                    node,
                                    umtsL3,
                                    UMTS_LAYER3_TIMER_CC_T308,
                                    UMTS_LAYER3_TIMER_CC_T308_NW_VAL,
                                    NULL,
                                    (void*)&timerInfo[0],
                                    index);
                ueCc->state = UMTS_CC_NW_RELEASE_REQUEST;
                ueCc->counterT308 = 0;
            }
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3MscHandleT308
// LAYER      :: Layer 3
// PURPOSE    :: Handle a CC T308 timer expired msg
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + gsnL3     : UmtsLayer3Gsn *  : UE data
// + msg       : Message*         : Message containing the packet
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3MscHandleT308(
        Node *node,
        UmtsLayer3Data *umtsL3,
        UmtsLayer3Gsn *gsnL3,
        Message *msg)
{
    char* timerInfo = MESSAGE_ReturnInfo(msg) + sizeof(UmtsLayer3Timer);
    NodeId ueId;
    char  ti;
    UmtsCallClearingType clearType;

    int index = 0;
    memcpy(&ueId, &timerInfo[index], sizeof(ueId));
    index += sizeof(ueId);
    ti = timerInfo[index ++];
    clearType = (UmtsCallClearingType)timerInfo[index++];

    std::list<UmtsSgsnUeDataInSgsn*>::iterator ueNasIt;
    ueNasIt = std::find_if(
                gsnL3->ueInfoList->begin(),
                gsnL3->ueInfoList->end(),
                UmtsFindItemByUeId<UmtsSgsnUeDataInSgsn>(ueId));

    if (ueNasIt != gsnL3->ueInfoList->end())
    {
        UmtsSgsnUeDataInSgsn* ueNas = *ueNasIt;
        UmtsLayer3SgsnFlowInfo* flowInfo =
            UmtsLayer3GsnFindAppInfoFromList(
                    node,
                    gsnL3,
                    (*ueNasIt),
                    ti);
        if (flowInfo)
        {
            UmtsCcNwData* ueCc = flowInfo->cmData.ccData;
            ueCc->T308 = NULL;
            if (ueCc->state == UMTS_CC_NW_RELEASE_REQUEST)
            {
                ueCc->counterT308++;
                if (ueCc->counterT308 < 2)
                {
                    UmtsLayer3MscSendReleaseToUe(node,
                                                 umtsL3,
                                                 (*ueNasIt),
                                                 flowInfo,
                                                 clearType);
                    char timerInfo[6];
                    memcpy(timerInfo, &(ueNas->ueId), sizeof(ueNas->ueId));
                    int index = sizeof(ueNas->ueId);
                    timerInfo[index++] = flowInfo->transId;
                    timerInfo[index++] = (char)clearType;
                    ueCc->T308 = UmtsLayer3SetTimer(
                                        node,
                                        umtsL3,
                                        UMTS_LAYER3_TIMER_CC_T308,
                                        UMTS_LAYER3_TIMER_CC_T308_NW_VAL,
                                        NULL,
                                        (void*)&timerInfo[0],
                                        index);
                }
                else
                {
                    //release RAB if allocated
                    if (flowInfo->rabId != UMTS_INVALID_RAB_ID)
                    {
                        UmtsLayer3MscSendVoiceCallRabRelease(
                            node,
                            umtsL3,
                            ueNas->ueId,
                            flowInfo->rabId);
                    }

                    ueCc->state = UMTS_CC_NW_NULL;
                    char ti = flowInfo->transId;
                    UmtsLayer3GsnRemoveAppInfoFromList(
                        node,
                        gsnL3,
                        ueNas->transIdBitSet,
                        ueNas->appInfoList,
                        flowInfo->srcDestType,
                        flowInfo->transId);
                    UmtsLayer3GsnReleaseMmConn(
                        node,
                        umtsL3,
                        ueNas,
                        UMTS_LAYER3_PD_CC,
                        ti);
                } // if (ueCc->counterT308)
            } // if (ueCc->state)
        } // if (flowInfo)
    } // if (ueNasIt)
}

// /**
// FUNCTION   :: UmtsLayer3MscHandleMobTermCallSetup
// LAYER      :: Layer 3
// PURPOSE    :: Handle a CALL SETUP message from the GMSC
// PARAMETERS ::
// + node           : Node*             : Pointer to node.
// + umtsL3         : UmtsLayer3Data*   : Pointer to the UMTS Layer3 data
// + ueId           : NodeId            : UE ID
// + packet         : char*             : Pointer to the message content
// + pktSize        : int               : Message size
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3MscHandleMobTermCallSetup(
    Node*                   node,
    UmtsLayer3Data*         umtsL3,
    NodeId                  ueId,
    char*                   packet,
    int                     pktSize)
{
    UmtsLayer3Gsn* gsnL3 = (UmtsLayer3Gsn*)(umtsL3->dataPtr);
    NodeId callingAddr;
    int appId;
    clocktype callEndTime;
    clocktype avgTalkTime;
    int index = 0;
    memcpy(&callingAddr, packet, sizeof(callingAddr));
    index += sizeof(callingAddr);
    memcpy(&appId, packet + index, sizeof(appId));
    index += sizeof(appId);
    memcpy(&callEndTime, packet + index, sizeof(callEndTime));
    index += sizeof(callEndTime);
    memcpy(&avgTalkTime, packet + index, sizeof(avgTalkTime));
    if (DEBUG_CC)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_CC);
        printf("\n\t MSC rcvd a CALL SETUP from a GMSC for UE: %d, "
            "calling UE: %d\n", ueId, callingAddr);
    }

    std::list<UmtsSgsnUeDataInSgsn*>::iterator ueNasIter
        = std::find_if(gsnL3->ueInfoList->begin(),
                       gsnL3->ueInfoList->end(),
                       UmtsFindItemByUeId
                            <UmtsSgsnUeDataInSgsn>(ueId));
    if (ueNasIter != gsnL3->ueInfoList->end())
    {
        UmtsSgsnUeDataInSgsn* ueNas = *ueNasIter;
        //UmtsMmNwData* ueMm = ueNas->mmData;
        int pos = UmtsGetPosForNextZeroBit<UMTS_MAX_TRANS_MOBILE>
                                    (*(ueNas->transIdBitSet), 0);
        if (!UmtsLayer3MscFindVoiceCallFlow(
                node, gsnL3, *ueNasIter, callingAddr, appId)
            && pos != -1)
        {
            char ti = (char)(pos + UMTS_LAYER3_NETWORK_MOBILE_START);
            ueNas->transIdBitSet->set(pos);

            UmtsLayer3FlowClassifier flowClsInfo;
            flowClsInfo.domainInfo = UMTS_LAYER3_CN_DOMAIN_CS;
            UmtsLayer3SgsnFlowInfo* flowInfo =
                       UmtsLayer3GsnAddAppInfoToList(
                           node,
                           gsnL3,
                           ueNas->appInfoList,
                           &flowClsInfo,
                           UMTS_FLOW_NETWORK_ORIGINATED,
                           ti,
                           0);
            UmtsCcNwData* ueCc = flowInfo->cmData.ccData;
            ueCc->peerAddr = callingAddr;
            ueCc->callAppId = appId;
            ueCc->endTime = callEndTime;
            ueCc->avgTalkTime = avgTalkTime;

            //establish MM connection to send CONNECT to UE
            if (UmtsLayer3GsnReqMmConnEst(node,
                                          umtsL3,
                                          ueNas,
                                          UMTS_LAYER3_PD_CC,
                                          ti))
            {
                //Send call setup to UE for mobile terminating call
                UmtsLayer3MscSendMobTermCallSetupToUe(
                        node,
                        umtsL3,
                        ueNas,
                        flowInfo);
                ueCc->state = UMTS_CC_NW_CALL_PRESENT;

                //Start to allocate radio channel for the call
                UmtsLayer3MscSendVoiceCallRabAssgt(
                        node,
                        umtsL3,
                        ueNas->ueId,
                        ti);
            }
            else
            {
                ueCc->state = UMTS_CC_NW_MM_CONNECTION_PENDING;
            }
        }
        else
        {
            //start call clearing toward GMSC
            //or do nothing to let calling side do the clearing?
            UmtsLayer3MscSendDisconnectToGmsc(
                node,
                umtsL3,
                ueId,
                0,
                UMTS_CALL_CLEAR_REJECT,
                callingAddr);
        }
    }
    else
    {
        //start to call clearing toward GMSC
        //or do nothing to let calling side do the clearing?
        UmtsLayer3MscSendDisconnectToGmsc(
            node,
            umtsL3,
            ueId,
            0,
            UMTS_CALL_CLEAR_REJECT,
            callingAddr);
    }
}

// /**
// FUNCTION   :: UmtsLayer3MscHandleAlertingFromGmsc
// LAYER      :: Layer 3
// PURPOSE    :: Handle an ALERTING  message from the GMSC
// PARAMETERS ::
// + node           : Node*             : Pointer to node.
// + umtsL3         : UmtsLayer3Data*   : Pointer to the UMTS Layer3 data
// + ueId           : NodeId            : UE ID
// + ti             : char              : transaction Id
// + packet         : char*             : Pointer to the message content
// + pktSize        : int               : Message size
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3MscHandleAlertingFromGmsc(
    Node*                   node,
    UmtsLayer3Data*         umtsL3,
    NodeId                  ueId,
    char                    ti,
    char*                   packet,
    int                     pktSize)
{
    UmtsLayer3Gsn* gsnL3 = (UmtsLayer3Gsn*)(umtsL3->dataPtr);
    if (DEBUG_CC)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_CC);
        printf("\n\t MSC rcvd an ALERTING from a GMSC for UE: %d,\n ",
            ueId);
    }

    std::list<UmtsSgsnUeDataInSgsn*>::iterator ueNasIter
        = std::find_if(gsnL3->ueInfoList->begin(),
                       gsnL3->ueInfoList->end(),
                       UmtsFindItemByUeId
                            <UmtsSgsnUeDataInSgsn>(ueId));
    if (ueNasIter != gsnL3->ueInfoList->end())
    {
        UmtsSgsnUeDataInSgsn* ueNas = *ueNasIter;
        UmtsLayer3SgsnFlowInfo* ueFlow = UmtsLayer3GsnFindAppInfoFromList(
                                            node,
                                            gsnL3,
                                            ueNas,
                                            ti);
        if (ueFlow && ueFlow->cmData.ccData->state
                        == UMTS_CC_NW_MOBILE_ORIGINATING_CALL_PROCEEDING)
        {
            UmtsLayer3MscSendAlertingToUe(
                node,
                umtsL3,
                ueNas,
                ueFlow);
            ueFlow->cmData.ccData->state
                = UMTS_CC_NW_CALL_DELIVERED;
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3MscHandleConnectFromGmsc
// LAYER      :: Layer 3
// PURPOSE    :: Handle a CONNECT message from the GMSC
// PARAMETERS ::
// + node           : Node*           : Pointer to node.
// + umtsL3         : UmtsLayer3Data* : Pointer to the UMTS Layer3 data
// + ueId           : NodeId          : UE ID
// + ti             : char            : transaction Id
// + packet         : char*           : Pointer to the message content
// + pktSize        : int             : Message size
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3MscHandleConnectFromGmsc(
    Node*                   node,
    UmtsLayer3Data*         umtsL3,
    NodeId                  ueId,
    char                    ti,
    char*                   packet,
    int                     pktSize)
{
    UmtsLayer3Gsn* gsnL3 = (UmtsLayer3Gsn*)(umtsL3->dataPtr);
    if (DEBUG_CC)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_CC);
        printf("\n\t MSC rcvd a CONNECT from a GMSC for UE: %d,\n ",
            ueId);
    }

    std::list<UmtsSgsnUeDataInSgsn*>::iterator ueNasIter
        = std::find_if(gsnL3->ueInfoList->begin(),
                       gsnL3->ueInfoList->end(),
                       UmtsFindItemByUeId
                            <UmtsSgsnUeDataInSgsn>(ueId));
    if (ueNasIter != gsnL3->ueInfoList->end())
    {
        UmtsSgsnUeDataInSgsn* ueNas = *ueNasIter;
        UmtsLayer3SgsnFlowInfo* ueFlow = UmtsLayer3GsnFindAppInfoFromList(
                                            node,
                                            gsnL3,
                                            ueNas,
                                            ti);
        if (ueFlow && (ueFlow->cmData.ccData->state
                        == UMTS_CC_NW_MOBILE_ORIGINATING_CALL_PROCEEDING
                   || ueFlow->cmData.ccData->state
                        == UMTS_CC_NW_CALL_DELIVERED))
        {
            UmtsCcNwData* ueCc = ueFlow->cmData.ccData;
            if (ueFlow->rabId != UMTS_INVALID_RAB_ID)
            {
                UmtsLayer3MscSendConnectToUe(
                    node,
                    umtsL3,
                    ueNas,
                    ueFlow);

                char timerInfo[5];
                memcpy(timerInfo, &(ueNas->ueId), sizeof(ueNas->ueId));
                int index = sizeof(ueNas->ueId);
                timerInfo[index++] = ueFlow->transId;
                ueCc->T313 = UmtsLayer3SetTimer(
                                    node,
                                    umtsL3,
                                    UMTS_LAYER3_TIMER_CC_T313,
                                    UMTS_LAYER3_TIMER_CC_T313_NW_VAL,
                                    NULL,
                                    (void*)&timerInfo[0],
                                    index);
                ueCc->state = UMTS_CC_NW_CONNECT_INDICATION;
            }
            else
            {
                UmtsCcNwData* ueCc = ueFlow->cmData.ccData;
                ueCc->state = UMTS_CC_NW_CONN_INDI_PENDING_ON_RAB_EST;
            }
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3MscHandleDisconnFromGmsc
// LAYER      :: Layer 3
// PURPOSE    :: Handle a DISCONNECT message from the GMSC
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data*   : Pointer to the UMTS Layer3 data
// + ueId           : NodeId                 : UE ID
// + ti             : char                   : transaction Id
// + packet         : char*             : Pointer to the message content
// + pktSize        : int                    : Message size
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3MscHandleDisconnFromGmsc(
    Node*                   node,
    UmtsLayer3Data*         umtsL3,
    NodeId                  ueId,
    char                    ti,
    char*                   packet,
    int                     pktSize)
{
    UmtsLayer3Gsn* gsnL3 = (UmtsLayer3Gsn*)(umtsL3->dataPtr);

    NodeId peerAddr;
    int appId;
    memcpy(&peerAddr, packet, sizeof(peerAddr));
    memcpy(&appId, packet + sizeof(peerAddr), sizeof(appId));
    UmtsCallClearingType clearType =
        (UmtsCallClearingType)
        (*(packet + sizeof(peerAddr) + sizeof(appId)));
    if (DEBUG_CC)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_CC);
        printf("\n\t MSC rcvd a DISCONNECT "
            "from a GMSC for UE: %d, TI: %d\n",
            ueId, ti);
    }

    std::list<UmtsSgsnUeDataInSgsn*>::iterator ueNasIter
        = std::find_if(gsnL3->ueInfoList->begin(),
                       gsnL3->ueInfoList->end(),
                       UmtsFindItemByUeId
                            <UmtsSgsnUeDataInSgsn>(ueId));
    if (ueNasIter != gsnL3->ueInfoList->end())
    {
        UmtsSgsnUeDataInSgsn* ueNas = *ueNasIter;
        UmtsLayer3SgsnFlowInfo* ueFlow = UmtsLayer3MscFindVoiceCallFlow(
                                            node,
                                            gsnL3,
                                            ueNas,
                                            peerAddr,
                                            appId);
        if (ueFlow)
        {
            UmtsCcNwData* ueCc = ueFlow->cmData.ccData;
            if (ueCc->state == UMTS_CC_NW_MM_CONNECTION_PENDING)
            {
                char ti = ueFlow->transId;
                if (ueFlow->srcDestType == UMTS_FLOW_NETWORK_ORIGINATED)
                {
                    (*ueNasIter)->transIdBitSet->
                        reset(ti - UMTS_LAYER3_NETWORK_MOBILE_START);
                }
                UmtsLayer3GsnAppInfoFinalize(node,
                                             gsnL3,
                                             ueFlow);
                (*ueNasIter)->appInfoList->remove(ueFlow);
            }
            else
            {
                UmtsLayer3MscInitCallClearing(
                    node,
                    umtsL3,
                    ueNas,
                    ueFlow,
                    clearType);
            }
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3GmscHandleCallSetup
// LAYER      :: Layer 3
// PURPOSE    :: Handle a CALL SETUP message from the MSC
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data* : Pointer to the UMTS Layer3 data
// + ueId           : NodeId                 : UE ID
// + ti             : char                   : Transaction ID
// + packet         : char*           : Pointer to the NAS message contents
// + pktSize        : int                    : Message size
// + srcMscAddr     : const Address&  : MSC address the message is from
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3GmscHandleCallSetup(
    Node*                   node,
    UmtsLayer3Data*         umtsL3,
    NodeId                  ueId,
    char                    ti,
    char*                   packet,
    int                     pktSize,
    const Address&          srcMscAddr)
{
    UmtsLayer3Gsn* gsnL3 = (UmtsLayer3Gsn*)(umtsL3->dataPtr);
    if (DEBUG_CC)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_CC);
        printf("\n\t GMSC rcvd a CALL SETUP "
            "from a MSC for UE: %d, TI: %d\n",
                ueId, ti);
    }

    NodeId calledAddr;
    int appId;
    clocktype endTime;
    clocktype avgTalkTime;
    int index = 0;
    memcpy(&calledAddr, packet, sizeof(calledAddr));
    index += sizeof(calledAddr);
    memcpy(&appId, packet + index, sizeof(appId));
    index += sizeof(appId);
    memcpy(&endTime, packet + index, sizeof(endTime));
    index += sizeof(endTime);
    memcpy(&avgTalkTime, packet + index, sizeof(avgTalkTime));

    UmtsLayer3GmscCallInfo* mobOrigCall =  UmtsLayer3GmscFindCallFlow(
                                                node,
                                                gsnL3,
                                                ueId,
                                                ti);
    if (mobOrigCall)
    {
        char warningMsg[MAX_STRING_LENGTH];
        sprintf(warningMsg, "A call flow with the same UE %d and TI: %d "
            "has been established in GMSC.\n", ueId, ti);
        ERROR_ReportError(warningMsg);
        return;
    }

    if (UmtsLayer3GmscIsCallAddrInMyPlmn(node, gsnL3, calledAddr))
    {
        UmtsLayer3GmscCallInfo* mobTermCall = UmtsLayer3GmscFindCallFlow(
                                                node,
                                                gsnL3,
                                                calledAddr,
                                                ueId);
        if (mobTermCall)
        {
            char warningMsg[MAX_STRING_LENGTH];
            sprintf(warningMsg, "A call flow between the calling UE %d "
                "and the called UE %d has been established in GMSC.\n",
                 ueId, calledAddr);
            ERROR_ReportWarning(warningMsg);
            return;
        }

        mobOrigCall = UmtsLayer3GmscAddCallFlow(node,
                                                gsnL3,
                                                ueId,
                                                calledAddr,
                                                appId,
                                                endTime,
                                                avgTalkTime,
                                                ti,
                                                &srcMscAddr);
        mobOrigCall->state = UMTS_CC_GMSC_CALL_INITIATED;

        mobTermCall = UmtsLayer3GmscAddCallFlow(node,
                                                gsnL3,
                                                calledAddr,
                                                ueId,
                                                appId,
                                                endTime,
                                                avgTalkTime);
        mobTermCall->state =
            UMTS_CC_GMSC_MOBILE_TERMINATING_ROUTING_PENDING;

        CellularIMSI calledImsi;
        calledImsi.setId(calledAddr);
        UmtsLayer3GsnQueryHlr(node, umtsL3, calledImsi);
    }
}

// /**
// FUNCTION   :: UmtsLayer3GmscHandleAlerting
// LAYER      :: Layer 3
// PURPOSE    :: Handle an ALERTING message from the MSC
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data*   : Pointer to the UMTS Layer3 data
// + ueId           : NodeId                 : UE ID
// + ti             : char                   : Transaction ID
// + packet         : char*        : Pointer to the NAS message contents
// + pktSize        : int                    : Message size
// + srcMscAddr     : const Address&    : MSC address the message is from
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3GmscHandleAlerting(
    Node*                   node,
    UmtsLayer3Data*         umtsL3,
    NodeId                  ueId,
    char                    ti,
    char*                   packet,
    int                     pktSize,
    const Address&          srcMscAddr)
{
    UmtsLayer3Gsn* gsnL3 = (UmtsLayer3Gsn*)(umtsL3->dataPtr);
    if (DEBUG_CC)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_CC);
        printf("\n\t GMSC rcvd an ALERTING "
            "from a MSC for UE: %d, TI: %d\n",
                ueId, ti);
    }

    NodeId callingAddr;
    memcpy(&callingAddr, packet, sizeof(callingAddr));

    UmtsLayer3GmscCallInfo* mobTermCall =  UmtsLayer3GmscFindCallFlow(
                                                node,
                                                gsnL3,
                                                ueId,
                                                callingAddr);
    if (!mobTermCall)
    {
        char warningMsg[MAX_STRING_LENGTH];
        sprintf(warningMsg, "A mobile terminating call flow with the UE %d "
            "and calling address: %d cannot be found in GMSC.\n",
            ueId, callingAddr);
        ERROR_ReportError(warningMsg);
        return;
    }

    mobTermCall->ti = ti;
    mobTermCall->state = UMTS_CC_GMSC_ALERTING;
    if (UmtsLayer3GmscIsCallAddrInMyPlmn(node, gsnL3, mobTermCall->peerAddr))
    {
        UmtsLayer3GmscCallInfo* mobOrigCall = UmtsLayer3GmscFindCallFlow(
                                                node,
                                                gsnL3,
                                                callingAddr,
                                                ueId);
        if (mobOrigCall)
        {
            UmtsLayer3GmscSendAlertingToMsc(node,
                                            umtsL3,
                                            callingAddr,
                                            mobOrigCall);
            mobTermCall->state = UMTS_CC_GMSC_ALERTING;
        }
    }
    else
    {
        //TODO: dealing with a PTSN node
    }
}

// /**
// FUNCTION   :: UmtsLayer3GmscHandleCallConnect
// LAYER      :: Layer 3
// PURPOSE    :: Handle an CONNECT message from the MSC originated from the
//               the call destination
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data*   : Pointer to the UMTS Layer3 data
// + ueId           : NodeId                 : UE ID
// + ti             : char                   : Transaction ID
// + packet         : char*        : Pointer to the NAS message contents
// + pktSize        : int                    : Message size
// + srcMscAddr     : const Address&   : MSC address the message is from
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3GmscHandleCallConnect(
    Node*                   node,
    UmtsLayer3Data*         umtsL3,
    NodeId                  ueId,
    char                    ti,
    char*                   packet,
    int                     pktSize,
    const Address&          srcMscAddr)
{
    UmtsLayer3Gsn* gsnL3 = (UmtsLayer3Gsn*)(umtsL3->dataPtr);
    if (DEBUG_CC)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_CC);
        printf("\n\t GMSC rcvd a CONNECT from a MSC for UE: %d, TI: %d\n",
                ueId, ti);
    }

    UmtsLayer3GmscCallInfo* mobTermCall =  UmtsLayer3GmscFindCallFlow(
                                                node,
                                                gsnL3,
                                                ueId,
                                                ti);
    if (!mobTermCall)
    {
        char warningMsg[MAX_STRING_LENGTH];
        sprintf(warningMsg, "A call flow with the UE %d and TI: %d "
            "cannot be found in GMSC.\n", ueId, ti);
        ERROR_ReportError(warningMsg);
        return;
    }

    if (UmtsLayer3GmscIsCallAddrInMyPlmn(
        node, gsnL3, mobTermCall->peerAddr))
    {
        UmtsLayer3GmscCallInfo* mobOrigCall = UmtsLayer3GmscFindCallFlow(
                                                node,
                                                gsnL3,
                                                mobTermCall->peerAddr,
                                                ueId);
        if (mobOrigCall)
        {
            UmtsLayer3GmscSendConnectToMsc(node,
                                           umtsL3,
                                           mobTermCall->peerAddr,
                                           mobOrigCall);
        }
    }
    else
    {
        //TODO: dealing with a PTSN node
    }
}

// /**
// FUNCTION   :: UmtsLayer3GmscHandleCallDisconn
// LAYER      :: Layer 3
// PURPOSE    :: Handle an DISCONNECT message from the MSC
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data*   : Pointer to the UMTS Layer3 data
// + ueId           : NodeId                 : UE ID
// + ti             : char                   : Transaction ID
// + packet         : char*        : Pointer to the NAS message contents
// + pktSize        : int                    : Message size
// + srcMscAddr     : const Address&   : MSC address the message is from
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3GmscHandleCallDisconn(
    Node*                   node,
    UmtsLayer3Data*         umtsL3,
    NodeId                  ueId,
    char                    ti,
    char*                   packet,
    int                     pktSize,
    const Address&          srcMscAddr)
{
    UmtsLayer3Gsn* gsnL3 = (UmtsLayer3Gsn*)(umtsL3->dataPtr);
    if (DEBUG_CC)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_CC);
        printf("\n\t GMSC rcvd a DISCONNECT "
            "from a MSC for UE: %d, TI: %d\n",
                ueId, ti);
    }

    UmtsCallClearingType clearType = (UmtsCallClearingType)(*packet);
    NodeId remoteAddr;
    memcpy(&remoteAddr, packet + sizeof(char), sizeof(remoteAddr));

    UmtsLayer3GmscCallInfo* clearingOrigCall =
                            UmtsLayer3GmscFindCallFlow(
                                node,
                                gsnL3,
                                ueId,
                                remoteAddr);

    if (!clearingOrigCall)
        return;
    if (UmtsLayer3GmscIsCallAddrInMyPlmn(node,
                                         gsnL3,
                                         clearingOrigCall->peerAddr))
    {
        UmtsLayer3GmscCallInfo* clearingTermCall
                            = UmtsLayer3GmscFindCallFlow(
                                                node,
                                                gsnL3,
                                                remoteAddr,
                                                ueId);
        if (clearingTermCall)
        {
            if (clearingTermCall->state >
                    UMTS_CC_GMSC_MOBILE_TERMINATING_ROUTING_PENDING)
            {
                UmtsLayer3GmscSendDisconnectToMsc(
                        node,
                        umtsL3,
                        remoteAddr,
                        clearingTermCall,
                        clearType);
            }
            gsnL3->callFlowList->remove(clearingTermCall);
            delete clearingTermCall;
        }
    }
    else
    {
        //TODO: dealing with a PTSN node
    }
    gsnL3->callFlowList->remove(clearingOrigCall);
    delete clearingOrigCall;
}

// /**
// FUNCTION   :: UmtsLayer3MscHandleCsSigMsg
// LAYER      :: Layer 3
// PURPOSE    :: Handle a backbone CS domain
//               signalling message from GGSN(GMSC) .
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data*   : Pointer to the UMTS Layer3 data
// + ueId           : NodeId                 : UE ID
// + ti             : char                   : Transaction ID
// + msgType        : UmtsCsSigMsgType       : CS msg type
// + packet         : char*        : Pointer to the NAS message contents
// + pktSize        : int                    : Message size
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3MscHandleCsSigMsg(
    Node*                   node,
    UmtsLayer3Data*         umtsL3,
    NodeId                  ueId,
    char                    ti,
    UmtsCsSigMsgType        msgType,
    char*                   packet,
    int                     pktSize)
{
    switch (msgType)
    {
        case UMTS_CSSIG_MSG_CALL_SETUP:
        {
            UmtsLayer3MscHandleMobTermCallSetup(node,
                                                umtsL3,
                                                ueId,
                                                packet,
                                                pktSize);
            break;
        }
        case UMTS_CSSIG_MSG_CALL_ALERTING:
        {
            UmtsLayer3MscHandleAlertingFromGmsc(node,
                                                umtsL3,
                                                ueId,
                                                ti,
                                                packet,
                                                pktSize);
            break;
        }
        case UMTS_CSSIG_MSG_CALL_CONNECT:
        {
            UmtsLayer3MscHandleConnectFromGmsc(node,
                                               umtsL3,
                                               ueId,
                                               ti,
                                               packet,
                                               pktSize);
            break;
        }
        case UMTS_CSSIG_MSG_CALL_DISCONNECT:
        {
            UmtsLayer3MscHandleDisconnFromGmsc(node,
                                               umtsL3,
                                               ueId,
                                               ti,
                                               packet,
                                               pktSize);
            break;
        }
        default:
            ERROR_ReportError(
                "Received an invalid CS DOMAIN signalling message.");
    }
}

// /**
// FUNCTION   :: UmtsLayer3GmscHandleCsSigMsg
// LAYER      :: Layer 3
// PURPOSE    :: Handle a backbone CS domain
//               signalling message from SGSN(MSC) .
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data*   : Pointer to the UMTS Layer3 data
// + ueId           : NodeId                 : UE ID
// + ti             : char                   : Transaction ID
// + msgType        : UmtsCsSigMsgType       : CS msg type
// + packet         : char*        : Pointer to the NAS message contents
// + pktSize        : int                    : Message size
// + srcMscAddr     : const Address&         : source MSC address
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3GmscHandleCsSigMsg(
    Node*                   node,
    UmtsLayer3Data*         umtsL3,
    NodeId                  ueId,
    char                    ti,
    UmtsCsSigMsgType        msgType,
    char*                   packet,
    int                     pktSize,
    const Address&          srcMscAddr)
{
    switch (msgType)
    {
        case UMTS_CSSIG_MSG_CALL_SETUP:
        {
            UmtsLayer3GmscHandleCallSetup(node,
                                          umtsL3,
                                          ueId,
                                          ti,
                                          packet,
                                          pktSize,
                                          srcMscAddr);
            break;
        }
        case UMTS_CSSIG_MSG_CALL_ALERTING:
        {
            UmtsLayer3GmscHandleAlerting(node,
                                         umtsL3,
                                         ueId,
                                         ti,
                                         packet,
                                         pktSize,
                                         srcMscAddr);
            break;
        }
        case UMTS_CSSIG_MSG_CALL_CONNECT:
        {
            UmtsLayer3GmscHandleCallConnect(node,
                                            umtsL3,
                                            ueId,
                                            ti,
                                            packet,
                                            pktSize,
                                            srcMscAddr);
            break;
        }
        case UMTS_CSSIG_MSG_CALL_DISCONNECT:
        {
            UmtsLayer3GmscHandleCallDisconn(node,
                                            umtsL3,
                                            ueId,
                                            ti,
                                            packet,
                                            pktSize,
                                            srcMscAddr);
            break;
        }
        default:
            ERROR_ReportError(
                "Received an invalid CS DOMAIN signalling message.");
    }
}

// /**
// FUNCTION   :: UmtsLayer3MscHandleCsPktFromRnc
// LAYER      :: Layer 3
// PURPOSE    :: Handle a backbone CS domain data packet from the RNC .
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data*   : Pointer to the UMTS Layer3 data
// + ueId           : NodeId                 : UE ID
// + rabId          : char              : RAB Id
// + msg            : Message*          : Message to be handled
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3MscHandleCsPktFromRnc(
    Node*                   node,
    UmtsLayer3Data*         umtsL3,
    NodeId                  ueId,
    char                    rabId,
    Message*                msg)
{
    UmtsLayer3Gsn* gsnL3 = (UmtsLayer3Gsn*)(umtsL3->dataPtr);
    if (DEBUG_CC_DATA)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_CC);
        printf("\n\t MSC rcvd a CS Data packet from a RNC for UE: %d, "
            "RAB ID: %d \n ",
            ueId, rabId);
    }

    BOOL msgReused = FALSE;
    std::list<UmtsSgsnUeDataInSgsn*>::iterator ueNasIter
        = std::find_if(gsnL3->ueInfoList->begin(),
                       gsnL3->ueInfoList->end(),
                       UmtsFindItemByUeId
                            <UmtsSgsnUeDataInSgsn>(ueId));
    if (ueNasIter != gsnL3->ueInfoList->end())
    {
        UmtsSgsnUeDataInSgsn* ueNas = *ueNasIter;
        UmtsLayer3SgsnFlowInfo* ueFlow = UmtsLayer3GsnFindAppByRabId(
                                            node,
                                            gsnL3,
                                            ueNas,
                                            rabId);
        if (ueFlow &&
            (ueFlow->cmData.ccData->state == UMTS_CC_NW_ACTIVE
             || ueFlow->cmData.ccData->state ==
             UMTS_CC_NW_CONNECT_INDICATION))
        {
            UmtsLayer3GsnSendCsPktOverBackbone(
                node,
                msg,
                gsnL3->ggsnAddr,
                ueFlow->transId,
                ueId);
            msgReused = TRUE;
            gsnL3->sgsnStat.numCsPktsToGmsc ++;
        }
    }
    if (msgReused == FALSE)
    {
        MESSAGE_Free(node, msg);
        gsnL3->sgsnStat.numCsPktsDropped ++;
    }
}

// /**
// FUNCTION   :: UmtsLayer3MscHandleCsPkt
// LAYER      :: Layer 3
// PURPOSE    :: Handle a backbone CS domain data packet from GGSN(GMSC) .
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data*   : Pointer to the UMTS Layer3 data
// + ueId           : NodeId                 : UE ID
// + ti             : char              : transaction Id
// + msg            : Message*          : Message to be handled
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3MscHandleCsPkt(
    Node*                   node,
    UmtsLayer3Data*         umtsL3,
    NodeId                  ueId,
    char                    ti,
    Message*                msg)
{
    UmtsLayer3Gsn* gsnL3 = (UmtsLayer3Gsn*)(umtsL3->dataPtr);
    if (DEBUG_CC_DATA)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_CC);
        printf("\n\t MSC rcvd a CS Data packet from a GMSC for UE: %d, "
            "TI: %d \n ",
            ueId, ti);
    }

    BOOL msgReused = FALSE;
    std::list<UmtsSgsnUeDataInSgsn*>::iterator ueNasIter
        = std::find_if(gsnL3->ueInfoList->begin(),
                       gsnL3->ueInfoList->end(),
                       UmtsFindItemByUeId
                            <UmtsSgsnUeDataInSgsn>(ueId));
    if (ueNasIter != gsnL3->ueInfoList->end())
    {
        UmtsSgsnUeDataInSgsn* ueNas = *ueNasIter;
        UmtsLayer3SgsnFlowInfo* ueFlow = UmtsLayer3GsnFindAppInfoFromList(
                                            node,
                                            gsnL3,
                                            ueNas,
                                            ti);
        if (ueFlow && (ueFlow->cmData.ccData->state == UMTS_CC_NW_ACTIVE))
        {
            NodeId rncId;
            NodeId cellId;
            UmtsLayer3AddIuCsDataHeader(
                node,
                msg,
                ueId,
                ueFlow->rabId);

            if (UmtsLayer3GsnLookupVlr(
                    node,
                    umtsL3,
                    ueId,
                    &rncId,
                    &cellId))
            {
                UmtsLayer3RncInfo* rncInfo = NULL;
                rncInfo = (UmtsLayer3RncInfo *)
                                FindItem(node,
                                         gsnL3->rncList,
                                         0,
                                         (char *) &(rncId),
                                         sizeof(rncId));
                ERROR_Assert(rncInfo, "cannot find RNC info. ");
                if (rncInfo->nodeAddr.networkType == NETWORK_IPV4)
                {
                    NetworkIpSendRawMessageToMacLayerWithDelay(
                        node,
                        msg,
                        NetworkIpGetInterfaceAddress(
                                node, rncInfo->interfaceIndex),
                        GetIPv4Address(rncInfo->nodeAddr),
                        IPTOS_PREC_INTERNETCONTROL,
                        IPPROTO_CELLULAR,
                        1,
                        rncInfo->interfaceIndex,
                        GetIPv4Address(rncInfo->nodeAddr),
                        10 * MILLI_SECOND);
                }
                else if (rncInfo->nodeAddr.networkType == NETWORK_IPV6)
                {
                    in6_addr srcAddr;
                    Ipv6GetGlobalAggrAddress(node,
                                             rncInfo->interfaceIndex,
                                             &srcAddr);
                    Ipv6SendRawMessageToMac(
                        node,
                        msg,
                        srcAddr,
                        GetIPv6Address(rncInfo->nodeAddr),
                        rncInfo->interfaceIndex,
                        IPTOS_PREC_INTERNETCONTROL,
                        IPPROTO_CELLULAR,
                        1);
                }

                msgReused = TRUE;
                gsnL3->sgsnStat.numCsPktsToUe ++;
            }
        }
    }
    if (msgReused == FALSE)
    {
        MESSAGE_Free(node, msg);
        gsnL3->sgsnStat.numCsPktsDropped ++;
    }
}

// /**
// FUNCTION   :: UmtsLayer3GmscHandleCsPkt
// LAYER      :: Layer 3
// PURPOSE    :: Handle a backbone CS domain data packet from SGSN(MSC) .
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data*   : Pointer to the UMTS Layer3 data
// + ueId           : NodeId                 : UE ID
// + ti             : char              : transaction Id
// + msg            : Message*          : Message to be handled
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3GmscHandleCsPkt(
    Node*                   node,
    UmtsLayer3Data*         umtsL3,
    NodeId                  ueId,
    char                    ti,
    Message*                msg)
{
    UmtsLayer3Gsn* gsnL3 = (UmtsLayer3Gsn*)(umtsL3->dataPtr);
    gsnL3->ggsnStat.numCsPktsFromInside ++;
    if (DEBUG_CC_DATA)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_CC);
        printf("\n\t GMSC rcvd a CS Data packet from a MSC for UE: %d, "
            "TI: %d \n ",
            ueId, ti);
    }

    BOOL msgReused = FALSE;
    UmtsLayer3GmscCallInfo* txFlow =  UmtsLayer3GmscFindCallFlow(
                                                node,
                                                gsnL3,
                                                ueId,
                                                ti);
    if (UmtsLayer3GmscIsCallAddrInMyPlmn(node, gsnL3, txFlow->peerAddr))
    {
        UmtsLayer3GmscCallInfo* rxFlow = UmtsLayer3GmscFindCallFlow(
                                                node,
                                                gsnL3,
                                                txFlow->peerAddr,
                                                ueId);
        if (txFlow && rxFlow)
        {
            //Set message protocol as CELLULAR
            //so it can not mistaken as IP packet
            //within cellular network
            MESSAGE_SetLayer(msg, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR);
            UmtsLayer3GsnSendCsPktOverBackbone(
                node,
                msg,
                rxFlow->mscAddr,
                rxFlow->ti,
                txFlow->peerAddr);
            msgReused = TRUE;
            gsnL3->ggsnStat.numCsPktsToInside ++;
        }
    }
    else
    {
        //TODO: dealing with a PTSN node
        gsnL3->ggsnStat.numCsPktsToOutside ++;
    }

    if (msgReused == FALSE)
    {
        MESSAGE_Free(node, msg);
        gsnL3->ggsnStat.numCsPktsDropped ++;
    }
}

// /**
// FUNCTION   :: UmtsLayer3GmscCheckCallPendingOnRouting
// LAYER      :: Layer 3
// PURPOSE    :: This function is called when getting routing info from HLR
// PARAMETERS ::
// + node      : Node*               : Pointer to node.
// + umtsL3    : UmtsLayer3Data *    : Pointer to UMTS Layer3 data
// + ueId      : NodeId              : UE ID
// + locInfo   : UmtsHlrLocationInfo*: Location info of the UE from HLR.
//                                     NULL means HLR lookup failed
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3GmscCheckCallPendingOnRouting(Node *node,
                                             UmtsLayer3Data *umtsL3,
                                             NodeId ueId,
                                             UmtsHlrLocationInfo *locInfo)
{
    UmtsLayer3Gsn *gsnL3 = (UmtsLayer3Gsn *) (umtsL3->dataPtr);
    UmtsGmscCallFlowList::iterator flowIter;

    //Assuming HLR inquiry has been made by one Call
    UmtsGmscCallFlowList::iterator mobTermCallIter =
        gsnL3->callFlowList->end();
    UmtsGmscCallFlowList::iterator mobOrigCallIter =
        gsnL3->callFlowList->end();
    for (flowIter = gsnL3->callFlowList->begin();
         flowIter != gsnL3->callFlowList->end();
         ++flowIter)
    {
        if ((*flowIter)->state ==
            UMTS_CC_GMSC_MOBILE_TERMINATING_ROUTING_PENDING &&
            (*flowIter)->ueId == ueId)
        {
            mobTermCallIter = flowIter;
            break;
        }
    }
    if (mobTermCallIter != gsnL3->callFlowList->end())
    {
        //If HLR has an entry for the callee
        if (locInfo != NULL)
        {
            (*mobTermCallIter)->state = UMTS_CC_GMSC_CALL_PRESENT;
            memcpy(&((*mobTermCallIter)->mscAddr),
                   &(locInfo->sgsnAddr),
                   sizeof(Address));
            //Start to sending message to MSC to establish
            //a mobile terminating call
            UmtsLayer3GmscSendCallSetupToMsc(
                node, umtsL3, ueId, (*mobTermCallIter));
        }
        //Otherwise, remove both call flow
        else
        {
            UmtsGmscCallFlowList::iterator callFlowIter
                            = gsnL3->callFlowList->begin();
            for (callFlowIter = gsnL3->callFlowList->begin();
                 callFlowIter != gsnL3->callFlowList->end();
                 callFlowIter++)
            {
                if ((*callFlowIter)->ueId == (*mobTermCallIter)->peerAddr
                    && (*callFlowIter)->peerAddr == (*mobTermCallIter)->ueId)
                {
                    mobOrigCallIter = callFlowIter;
                    break;
                }
            }
            ERROR_Assert(mobOrigCallIter != gsnL3->callFlowList->end(),
                "UmtsLayer3GmscCheckCallPendingOnRouting: can't find "
                "corresponding mobile originated call.");
            UmtsLayer3GmscSendDisconnectToMsc(
                    node,
                    umtsL3,
                    (*mobOrigCallIter)->ueId,
                    *mobOrigCallIter,
                    UMTS_CALL_CLEAR_REJECT);

            delete *mobOrigCallIter;
            delete *mobTermCallIter;
            *mobOrigCallIter = NULL;
            *mobTermCallIter = NULL;
            gsnL3->callFlowList->remove(NULL);
        }
    }
}

/////////
// handle NAS  msg
////////
// /**
// FUNCTION   :: UmtsLayer3GsnHandleNasMsg
// LAYER      :: Layer 3
// PURPOSE    :: Handle a NAS stratum message from a UE.
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msg            : char*      : Pointer to the NAS message contents
// + msgSize        : int                    : Message size
// + ueId           : NodeId                 : UE ID
// + rncId          : NodeId                 : RNC ID
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3GsnHandleNasMsg(
    Node*           node,
    UmtsLayer3Data* umtsL3,
    char*           msg,
    int             msgSize,
    NodeId          ueId,
    NodeId          rncId)
{
    UmtsLayer3Gsn *gsnL3 = (UmtsLayer3Gsn *) (umtsL3->dataPtr);

    UmtsLayer3Header l3Header;
    memcpy(&l3Header, msg, sizeof(UmtsLayer3Header));

    std::list<UmtsSgsnUeDataInSgsn*>::iterator ueNasIter
        = std::find_if(gsnL3->ueInfoList->begin(),
                       gsnL3->ueInfoList->end(),
                       UmtsFindItemByUeId
                            <UmtsSgsnUeDataInSgsn>(ueId));

    if (ueNasIter != gsnL3->ueInfoList->end()
        && (l3Header.pd == UMTS_LAYER3_PD_CC
       /* || l3Header.pd == UMTS_LAYER3_PD_GCC
        || l3Header.pd == UMTS_LAYER3_PD_BCC
        || l3Header.pd == UMTS_LAYER3_PD_SM
        || l3Header.pd == UMTS_LAYER3_PD_LS */))
    {
        UmtsMmNwData* ueMm = (*ueNasIter)->mmData;
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
        UmtsLayer3GsnMmConnEstInd(node, umtsL3, *ueNasIter);
        if (ueMm->mainState != UMTS_MM_NW_CONNECTION_ACTIVE)
        {
            ueMm->mainState = UMTS_MM_NW_CONNECTION_ACTIVE;
        }
    }

    switch (l3Header.pd)
    {
        case UMTS_LAYER3_PD_CC:
        {
            if (ueNasIter != gsnL3->ueInfoList->end())
            {
                UmtsLayer3MscHandleCcMsg(
                    node,
                    umtsL3,
                    *ueNasIter,
                    (UmtsCcMessageType)l3Header.msgType,
                    (char)l3Header.tiSpd,
                    msg + sizeof(UmtsLayer3Header),
                    msgSize - sizeof(UmtsLayer3Header));
            }
            break;
        }
        case UMTS_LAYER3_PD_MM:
        {
            UmtsLayer3GsnHandleMmMsg(
                node,
                umtsL3,
                (UmtsMmMessageType)l3Header.msgType,
                (char)l3Header.tiSpd,
                msg + sizeof(UmtsLayer3Header),
                msgSize - sizeof(UmtsLayer3Header),
                ueId,
                rncId);
            break;
        }
        case UMTS_LAYER3_PD_GMM:
        {
            UmtsLayer3GsnHandleGMmMsg(
                node,
                umtsL3,
                (UmtsGMmMessageType)l3Header.msgType,
                (char)l3Header.tiSpd,
                msg + sizeof(UmtsLayer3Header),
                msgSize - sizeof(UmtsLayer3Header),
                ueId,
                rncId);
            break;
        }
        case UMTS_LAYER3_PD_SM:
        {
             UmtsLayer3GsnHandleSmMsg(
                node,
                umtsL3,
                (UmtsSmMessageType)l3Header.msgType,
                (char)l3Header.tiSpd,
                msg + sizeof(UmtsLayer3Header),
                msgSize - sizeof(UmtsLayer3Header),
                ueId,
                rncId);
            break;
        }
        default:
        {
            char errStr[MAX_STRING_LENGTH];
            sprintf(errStr,
                    "UMTS Layer3: SGSN: %d receivers a layer3 "
                    "signalling msg with unknown type pd %d",
                    node->nodeId,
                    l3Header.pd);
            ERROR_ReportWarning(errStr);
            break;
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3GsnHandleRanapInitUeMsg
// LAYER      :: Layer 3
// PURPOSE    :: Handle a Initial UE RANAP message from one RNC.
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msg            : Message*               : Message containing the packet
// + ueId           : NodeId                 : UE ID
// + rncId          : NodeId                 : RNC ID
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3GsnHandleRanapInitUeMsg(
    Node*           node,
    UmtsLayer3Data* umtsL3,
    Message*        msg,
    NodeId          ueId,
    NodeId          rncId)
{
    char* packet = MESSAGE_ReturnPacket(msg);
    char* nasMsg = packet + 1;
    int nasMsgSize = MESSAGE_ReturnPacketSize(msg) - 1;

    UmtsLayer3CnDomainId domain;

    domain = (UmtsLayer3CnDomainId)(*packet);
    UmtsLayer3GsnHandleNasMsg(
        node,
        umtsL3,
        nasMsg,
        nasMsgSize,
        ueId,
        rncId);
}

// /**
// FUNCTION   :: UmtsLayer3GsnHandleRanapDirectTransfer
// LAYER      :: Layer 3
// PURPOSE    :: Handle a Direct Transfer RANAP message from one RNC.
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msg            : Message*               : Message containing the packet
// + ueId           : NodeId                 : UE ID
// + rncId          : NodeId                 : RNC ID
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3GsnHandleRanapDirectTransfer(
    Node*           node,
    UmtsLayer3Data* umtsL3,
    Message*        msg,
    NodeId          ueId,
    NodeId          rncId)
{
    char* packet = MESSAGE_ReturnPacket(msg);
    char* nasMsg = packet + 1;
    int nasMsgSize = MESSAGE_ReturnPacketSize(msg) - 1;

    UmtsLayer3CnDomainId domain;

    domain = (UmtsLayer3CnDomainId)(*packet);
    UmtsLayer3GsnHandleNasMsg(
        node,
        umtsL3,
        nasMsg,
        nasMsgSize,
        ueId,
        rncId);
}

// /**
// FUNCTION   :: UmtsLayer3GsnHandleRabAssgtRes
// LAYER      :: Layer 3
// PURPOSE    :: Handle a RAB_ASSIGNMENT_RESPONSE for voice call set up
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msg            : Message*               : Message containing the packet
// + ueId           : NodeId                 : UE ID
// + rncId          : NodeId                 : RNC ID
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3MscHandleCallRabAssgtRes(
    Node* node,
    UmtsLayer3Data* umtsL3,
    UmtsSgsnUeDataInSgsn* ueNas,
    char ti,
    UmtsRabAssgtResTlvType tlvType,
    char* packet,
    int& index)
{
    UmtsLayer3Gsn* gsnL3 = (UmtsLayer3Gsn*)(umtsL3->dataPtr);
    UmtsLayer3SgsnFlowInfo* ueFlow =
        UmtsLayer3GsnFindAppInfoFromList(node,
                                         gsnL3,
                                         ueNas,
                                         ti);
    if (tlvType == TLV_RABASSGTRES_SETUP)
    {
        char rabId = packet[index ++];
        if (ueFlow)
        {
            ueFlow->rabId = rabId;
            UmtsCcNwData* ueCc = ueFlow->cmData.ccData;
            if (ueCc->state == UMTS_CC_NW_ACTIVE_PENDING_ON_RAB_EST)
            {
                UmtsLayer3MscSendConnAckToUe(
                    node,
                    umtsL3,
                    ueNas,
                    ueFlow);
                UmtsLayer3MscSendMobTermCallConnectToGmsc(
                    node,
                    umtsL3,
                    ueNas,
                    ueFlow);
                ueCc->state = UMTS_CC_NW_ACTIVE;
            }
            else if (ueCc->state == UMTS_CC_NW_CONN_INDI_PENDING_ON_RAB_EST)
            {
                UmtsLayer3MscSendConnectToUe(
                    node,
                    umtsL3,
                    ueNas,
                    ueFlow);

                char timerInfo[5];
                memcpy(timerInfo,
                       &(ueNas->ueId),
                       sizeof(ueNas->ueId));
                int tidx = sizeof(ueNas->ueId);
                timerInfo[tidx ++] = ueFlow->transId;
                ueCc->T313 = UmtsLayer3SetTimer(
                                    node,
                                    umtsL3,
                                    UMTS_LAYER3_TIMER_CC_T313,
                                    UMTS_LAYER3_TIMER_CC_T313_NW_VAL,
                                    NULL,
                                    (void*)&timerInfo[0],
                                    tidx);
                ueCc->state = UMTS_CC_NW_CONNECT_INDICATION;
            }
            else if (ueCc->state == UMTS_CC_NW_NULL)
            {
                UmtsLayer3MscSendVoiceCallRabRelease(
                    node,
                    umtsL3,
                    ueNas->ueId,
                    rabId);
            }
        }
        else
        {
            UmtsLayer3MscSendVoiceCallRabRelease(
                node,
                umtsL3,
                ueNas->ueId,
                rabId);
        }
    }
    else
    {
        if (ueFlow)
        {
            UmtsCcNwData* ueCc = ueFlow->cmData.ccData;
            //clearing CC in network and mobile
            UmtsLayer3MscInitCallClearing(
                node,
                umtsL3,
                ueNas,
                ueFlow,
                UMTS_CALL_CLEAR_REJECT);

            if (ueCc->state != UMTS_CC_NW_NULL
                && ueCc->state != UMTS_CC_NW_DISCONNECT_INDICATION
                && ueCc->state != UMTS_CC_NW_RELEASE_REQUEST)
            {
                UmtsLayer3MscSendDisconnectToGmsc(
                    node,
                    umtsL3,
                    ueNas->ueId,
                    ueFlow->transId,
                    UMTS_CALL_CLEAR_REJECT,
                    ueFlow->cmData.ccData->peerAddr);
            }
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3GsnHandleRabAssgtRes
// LAYER      :: Layer 3
// PURPOSE    :: Handle a RAB_ASSIGNMENT_RESPONSE message.
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msg            : Message*               : Message containing the packet
// + ueId           : NodeId                 : UE ID
// + rncId          : NodeId                 : RNC ID
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3GsnHandleRabAssgtRes(
    Node*           node,
    UmtsLayer3Data* umtsL3,
    Message*        msg,
    NodeId          ueId,
    NodeId          rncId)
{
    UmtsLayer3Gsn *gsnL3 = (UmtsLayer3Gsn *) (umtsL3->dataPtr);
    char* packet = MESSAGE_ReturnPacket(msg);

    int index = 0;
    while (index < MESSAGE_ReturnPacketSize(msg))
    {
        UmtsRabAssgtResTlvType tlvType = (UmtsRabAssgtResTlvType)
                                         packet[index ++];
        switch (tlvType)
        {
            case TLV_RABASSGTRES_SETUP:
            case TLV_RABASSGTRES_FAIL_SETUP:
            {
                UmtsLayer3CnDomainId domain =
                    (UmtsLayer3CnDomainId) packet[index ++];
                UmtsRabConnMap connMap;
                memcpy(&(connMap), &packet[index], sizeof(connMap));
                index += sizeof(connMap);
                char rabId = UMTS_INVALID_RAB_ID;

                std::list<UmtsSgsnUeDataInSgsn*>::iterator ueNasIt;
                ueNasIt = std::find_if(
                            gsnL3->ueInfoList->begin(),
                            gsnL3->ueInfoList->end(),
                            UmtsFindItemByUeId<UmtsSgsnUeDataInSgsn>(ueId));
                if (ueNasIt == gsnL3->ueInfoList->end())
                {
                    return;
                }

                if (domain == UMTS_LAYER3_CN_DOMAIN_PS)
                {
                    std::list<UmtsLayer3SgsnFlowInfo*>::iterator appIt;
                    appIt = std::find_if(
                                (*ueNasIt)->appInfoList->begin(),
                                (*ueNasIt)->appInfoList->end(),
                              UmtsFindItemByDlTeId<UmtsLayer3SgsnFlowInfo>(
                              (unsigned int)(connMap.tunnelMap.dlTeId)));

                    if (appIt == (*ueNasIt)->appInfoList->end())
                    {
                        return;
                    }

                    if (tlvType == TLV_RABASSGTRES_SETUP)
                    {
                        // get the assigned RAB ID
                        rabId = packet[index ++];

                        (*appIt)->rabId = rabId;

                        // Send Accept msg to UE
                        UmtsLayer3GsnSmSendActivatePDPContextAccept(node,
                                                                    umtsL3,
                                                                    gsnL3,
                                                                    *appIt,
                                                                    rabId,
                                                                    ueId);

                        // update stat
                        gsnL3->sgsnStat.numActivatePDPContextAcptSent ++;

                        // move state to active
                        (*appIt)->cmData.smData->smState =
                            UMTS_SM_NW_PDP_ACTIVE;
                    }
                    else
                    {
                        // Send Reject msg to UE
                        UmtsLayer3GsnSmSendActivatePDPContextReject(
                            node,
                            umtsL3,
                            gsnL3,
                            *appIt,
                            UMTS_SM_CAUSE_INSUFFICINT_RESOURCES);

                        // update stat
                        gsnL3->sgsnStat.numActivatePDPContextRjtSent ++;

                        // Remove the SM session as the
                        //activation request is rejected
                    }

                    if ((*appIt)->srcDestType ==
                        UMTS_FLOW_NETWORK_ORIGINATED)
                    {
                        // send PDUNotification Response back to GGSN
                        UmtsLayer3GsnGtpSendPduNotificationResponse(
                            node,
                            umtsL3,
                            gsnL3,
                            (*ueNasIt),
                            (*appIt),
                            tlvType == TLV_RABASSGTRES_SETUP ?
                                UMTS_GTP_REQUEST_ACCEPTED :
                                UMTS_GTP_REQUEST_REJECTED,
                            gsnL3->ggsnAddr);

                        // update stat
                        gsnL3->sgsnStat.numPduNotificationRspSent ++;
                    }
                }
                //If the RAB is used for a voice call
                else if (domain == UMTS_LAYER3_CN_DOMAIN_CS
                        && connMap.flowMap.pd == UMTS_LAYER3_PD_CC)
                {
                    UmtsLayer3MscHandleCallRabAssgtRes(
                        node,
                        umtsL3,
                        *ueNasIt,
                        connMap.flowMap.ti,
                        tlvType,
                        packet,
                        index);
                }
                break;
            }
            case TLV_RABASSGTRES_RELEASE:
            {
                UmtsLayer3CnDomainId domain;

                domain = (UmtsLayer3CnDomainId) packet[index ++];
                UmtsRabConnMap connMap;
                memcpy(&(connMap), &packet[index], sizeof(connMap));
                index += sizeof(connMap);
                break;
            }
            case TLV_RABASSGTRES_FAIL_RELEASE:
            {
                UmtsLayer3CnDomainId domain;

                domain = (UmtsLayer3CnDomainId) packet[index ++];
                UmtsRabConnMap connMap;
                memcpy(&(connMap), &packet[index], sizeof(connMap));
                index += sizeof(connMap);
                break;
            }
            default:
                ERROR_ReportError("Unexpected TLV type is found "
                    "int the received RAB_ASSIGNMENT_RESPONSE message.");
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3GsnHandleIuReleaseReport
// LAYER      :: Layer 3
// PURPOSE    :: Handle a IU RELEASE reported by the RNC.
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msg            : Message*               : Message containing the packet
// + ueId           : NodeId                 : UE ID
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3GsnHandleIuReleaseReport(
    Node*           node,
    UmtsLayer3Data* umtsL3,
    Message*        msg,
    NodeId          ueId)
{
    UmtsLayer3Gsn *gsnL3 = (UmtsLayer3Gsn *) (umtsL3->dataPtr);

    UmtsLayer3CnDomainId domain = (UmtsLayer3CnDomainId)
                                  (*MESSAGE_ReturnPacket(msg));


    UmtsLayer3GsnSigConnRelInd(
        node,
        umtsL3,
        gsnL3,
        ueId,
        domain);
}

// /**
// FUNCTION   :: UmtsLayer3GsnHandleCellLookup
// LAYER      :: Layer 3
// PURPOSE    :: Handle a RAB_ASSIGNMENT_RESPONSE message.
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msg            : Message*               : Message containing the packet
// + rncId          : NodeId                 : RNC ID
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3GsnHandleCellLookup(
    Node*           node,
    UmtsLayer3Data* umtsL3,
    Message*        msg,
    NodeId          rncId)
{
    UmtsLayer3Gsn *gsnL3 = (UmtsLayer3Gsn *) (umtsL3->dataPtr);
    char* packet = MESSAGE_ReturnPacket(msg);

    UInt32 cellId;
    memcpy(&cellId, packet, sizeof(cellId));

    UmtsCellCacheMap::iterator it =
        gsnL3->cellCacheMap->find(cellId);
    if (it != gsnL3->cellCacheMap->end())
    {
        UmtsLayer3GsnSendCellLookupReply(
            node,
            umtsL3,
            cellId,
            it->second->nodebId,
            it->second->rncId,
            rncId);
    }
    else
    {
        UmtsLayer3GsnQueryHlrCellInfo(
            node,
            umtsL3,
            cellId,
            rncId);
    }
}

// /**
// FUNCTION   :: UmtsLayer3GsnHandleIurForwardMsg
// LAYER      :: Layer 3
// PURPOSE    :: Handle a Iur message encasulated in the RANAP message.
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msg            : Message*               : Message containing the packet
// + rncId          : NodeId                 : RNC ID
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3GsnHandleIurForwardMsg(
    Node*           node,
    UmtsLayer3Data* umtsL3,
    Message*        msg,
    NodeId          destRncId)
{
    UmtsLayer3Gsn *gsnL3 = (UmtsLayer3Gsn *) (umtsL3->dataPtr);

    UmtsRncCacheInfo* destRncInfo =
        gsnL3->rncCacheMap->find(destRncId)->second;

    if (DEBUG_IUR)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_RRC);
        printf("\n\treceives an IUR Forward message to RNC: %u \n",
            destRncId);
        UmtsPrintMessage(std::cout, msg);
    }

    if (destRncInfo->sgsnId == node->nodeId)
    {
        UmtsLayer3SgsnSendRanapMsgToRnc(
            node,
            umtsL3,
            msg,
            destRncId,
            UMTS_RANAP_MSG_TYPE_IUR_MESSAGE_FORWARD,
            destRncId);
    }
    else
    {
        //forward the message to the SGSN where the
        //target RNC is located
        UmtsLayer3SgsnSendIurForwardMsgToSgsn(
            node,
            umtsL3,
            msg,
            destRncId,
            destRncInfo->sgsnAddr);
    }
}

// /**
// FUNCTION   :: UmtsLayer3GsnHandleRanapMsg
// LAYER      :: Layer 3
// PURPOSE    :: Handle a RANAP message from one RNC.
// PARAMETERS ::
// + node           : Node*                  : Pointer to node.
// + umtsL3         : UmtsLayer3Data *       : Pointer to UMTS Layer3 data
// + msg            : Message*               : Message containing the packet
// + msgType        : UmtsRanapMessageType   : RANAP message type
// + NodeId         : ueId                   : UE ID
// + interfaceIndex : int     : Interface from which packet was received
// + srcAddr        : Address                : Address of the source node
// RETURN           :: void : NULL
// **/
static
void UmtsLayer3GsnHandleRanapMsg(
        Node *node,
        UmtsLayer3Data *umtsL3,
        Message *msg,
        UmtsRanapMessageType msgType,
        NodeId nodeId,
        int interfaceIndex,
        Address srcAddr)
{
    UmtsLayer3Gsn *gsnL3 = (UmtsLayer3Gsn *) (umtsL3->dataPtr);

    UmtsLayer3RncInfo *itemPtr = NULL;
    itemPtr = (UmtsLayer3RncInfo *)
              FindItem(node,
                       gsnL3->rncList,
                       sizeof(NodeId) + sizeof(Address),
                       (char *)&interfaceIndex,
                       sizeof(int));
    ERROR_Assert(itemPtr, "UmtsLayer3GsnHandleRanapMsg: "
        "cannot find RNC information for the source of the message.");
    NodeId rncId = itemPtr->nodeId;

    switch (msgType)
    {
        case UMTS_RANAP_MSG_TYPE_INITIAL_UE_MESSAGE:
        {
            NodeId ueId = nodeId;
            UmtsLayer3GsnHandleRanapInitUeMsg(
                node,
                umtsL3,
                msg,
                ueId,
                rncId);
            MESSAGE_Free(node, msg);
            break;
        }
        case UMTS_RANAP_MSG_TYPE_DIRECT_TRANSFER:
        {
            NodeId ueId = nodeId;
            UmtsLayer3GsnHandleRanapDirectTransfer(
                node,
                umtsL3,
                msg,
                ueId,
                rncId);
            MESSAGE_Free(node, msg);
            break;
        }
        case UMTS_RANAP_MSG_TYPE_RAB_ASSIGNMENT_RESPONSE:
        {
            NodeId ueId = nodeId;
            UmtsLayer3GsnHandleRabAssgtRes(
                node,
                umtsL3,
                msg,
                ueId,
                rncId);
            MESSAGE_Free(node, msg);
            break;
        }
        case UMTS_RANAP_MSG_TYPE_CELL_LOOKUP:
        {
            UmtsLayer3GsnHandleCellLookup(
                node,
                umtsL3,
                msg,
                rncId);
            MESSAGE_Free(node, msg);
            break;
        }
        case UMTS_RANAP_MSG_TYPE_IUR_MESSAGE_FORWARD:
        {
            NodeId destRncId = nodeId;
            UmtsLayer3GsnHandleIurForwardMsg(
                node,
                umtsL3,
                msg,
                destRncId);
            //message cannot be freed since it will be forwarded
            break;
        }
        case UMTS_RANAP_MSG_TYPE_IU_RELEASE_COMPLETE:
        {
            //Do nothing
            break;
        }
        case UMTS_RANAP_MSG_TYPE_IU_RELEASE_REQUEST:
        {
            NodeId ueId = nodeId;
            UmtsLayer3GsnHandleIuReleaseReport(
                node,
                umtsL3,
                msg,
                ueId);
            MESSAGE_Free(node, msg);
            break;
        }
        default:
        {
            char infoStr[MAX_STRING_LENGTH];
            sprintf(infoStr,
                    "UMTS Layer3: Node%d(GSN) receives a RANAP message "
                    "with an unknown RANAP message type as %d",
                    node->nodeId, msgType);
            ERROR_ReportWarning(infoStr);
        }
    }
}

///////
// Handle GTP msg
///////
// /**
// FUNCTION   :: UmtsLayer3GgsnAddPdpInfoToList
// LAYER      :: Layer 3
// PURPOSE    :: Handle a GTP msg at SGSN
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + gsnL3     : UmtsLayer3Gsn*   : GSN layer 3 data
// + nsapi     : unsigned char    : NSAPI of the PDP
// + dlTeId    : UInt32           : dl TEID
// + ulTeId    : UInt32           : ul TEID
// + qosInfo   : UmtsFlowQoSInfo* : QoS Info
// RETURN     ::  UmtsLayer3GgsnPdpInfo* : Newly added PDP info
// **/
static
UmtsLayer3GgsnPdpInfo* UmtsLayer3GgsnAddPdpInfoToList(
                           Node *node,
                           UmtsLayer3Data *umtsL3,
                           UmtsLayer3Gsn* gsnL3,
                           unsigned char* nsapi,
                           UInt32 *dlTeId,
                           UInt32 *ulTeId,
                           UmtsFlowQoSInfo* qosInfo)
{
    UmtsLayer3GgsnPdpInfo* pdpInfo;
    pdpInfo = new UmtsLayer3GgsnPdpInfo;
    if (nsapi)
    {
        pdpInfo->nsapi = *nsapi;
    }
    if (dlTeId)
    {
        pdpInfo->dlTeId = *dlTeId;
    }
    else
    {
        pdpInfo->dlTeId = (UInt32) -1;
    }
    pdpInfo->ulTeId = *ulTeId; // UL TEID is generated by GGSN

    // clssifier
    pdpInfo->classifierInfo = (UmtsLayer3FlowClassifier*)
                             MEM_malloc(sizeof(UmtsLayer3FlowClassifier));
    memset(pdpInfo->classifierInfo, 0, sizeof(UmtsLayer3FlowClassifier));

    // QoS info
    pdpInfo->qosInfo =
        (UmtsFlowQoSInfo*)MEM_malloc(sizeof(UmtsFlowQoSInfo));
    memset((void*)(pdpInfo->qosInfo), 0, sizeof(UmtsFlowQoSInfo));
    if (qosInfo)
    {
        memcpy((void*)(pdpInfo->qosInfo),
               (void*)qosInfo,
               sizeof(UmtsFlowQoSInfo));
    }

    gsnL3->pdpInfoList->push_back(pdpInfo);

    return pdpInfo;
}

// /**
// FUNCTION   :: UmtsLayer3GgsnFindPdpInfoByTeId
// LAYER      :: Layer 3
// PURPOSE    :: Handle a GTP msg at SGSN
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + gsnL3     : UmtsLayer3Gsn*   : GSN layer 3 data
// + teId      : UInt32           : Te Id
// + ueId      : NodeId*          : UE Id
// + upLink    : BOOL             : Is it uplink
// RETURN     :: UmtsLayer3GgsnPdpInfo* : Founded PDP Info
// **/
static
UmtsLayer3GgsnPdpInfo* UmtsLayer3GgsnFindPdpInfoByTeId(
                           Node *node,
                           UmtsLayer3Data *umtsL3,
                           UmtsLayer3Gsn* gsnL3,
                           UInt32 teId,
                           NodeId* ueId,
                           BOOL upLink)
{
    std::list<UmtsLayer3GgsnPdpInfo*>::iterator it;

    UmtsLayer3GgsnPdpInfo* pdpInfo = NULL;
    if (upLink)
    {
        for (it = gsnL3->pdpInfoList->begin();
            it != gsnL3->pdpInfoList->end();
            it ++)
        {
            if ((*it)->ulTeId == teId)
            {
                break;
            }
        }
    }
    else
    {
        for (it = gsnL3->pdpInfoList->begin();
            it != gsnL3->pdpInfoList->end();
            it ++)
        {
            if ((*it)->dlTeId == teId &&
                (*it)->ueId == (*ueId))
            {
                break;
            }
        }
    }

    if (it != gsnL3->pdpInfoList->end())
    {
        pdpInfo = (*it);
    }

    return pdpInfo;
}

#if 0
// /**
// FUNCTION   :: UmtsLayer3GgsnFindPdpInfoByNsapi
// LAYER      :: Layer 3
// PURPOSE    :: Handle a GTP msg at SGSN
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + gsnL3     : UmtsLayer3Gsn*   : GSN layer 3 data
// + nsapi     : unsigned char    : NSAPI of the PDP
// + ueId      : NodeId           : NodeId of UE
// RETURN     :: UmtsLayer3GgsnPdpInfo* : PDP Info
// **/
static
UmtsLayer3GgsnPdpInfo* UmtsLayer3GgsnFindPdpInfoByNsapi(
                           Node *node,
                           UmtsLayer3Data *umtsL3,
                           UmtsLayer3Gsn* gsnL3,
                           unsigned char nsapi,
                           NodeId ueId)
{
    std::list<UmtsLayer3GgsnPdpInfo*>::iterator it;

    UmtsLayer3GgsnPdpInfo* pdpInfo = NULL;
    for (it = gsnL3->pdpInfoList->begin();
        it != gsnL3->pdpInfoList->end();
        it ++)
    {
        if ((*it)->nsapi == nsapi &&
            (*it)->ueId == ueId)
        {
            break;
        }
    }

    if (it != gsnL3->pdpInfoList->end())
    {
        pdpInfo = (*it);
    }

    return pdpInfo;
}
#endif

// /**
// FUNCTION   :: UmtsLayer3GgsnRemovePdpInfoByTeId
// LAYER      :: Layer 3
// PURPOSE    :: Handle a GTP msg at SGSN
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + gsnL3     : UmtsLayer3Gsn*   : GSN layer 3 data
// + teId      : UInt32           : Te Id
// + ueId      : NodeId*          : UE Id
// + upLink    : BOOL             : Is it uplink
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3GgsnRemovePdpInfoByTeId(
                           Node *node,
                           UmtsLayer3Data *umtsL3,
                           UmtsLayer3Gsn* gsnL3,
                           UInt32 teId,
                           NodeId* ueId,
                           BOOL upLink)
{
    std::list<UmtsLayer3GgsnPdpInfo*>::iterator it;

    if (upLink)
    {
        for (it = gsnL3->pdpInfoList->begin();
            it != gsnL3->pdpInfoList->end();
            it ++)
        {
            if ((*it)->ulTeId == teId)
            {
                break;
            }
        }
    }
    else
    {
        for (it = gsnL3->pdpInfoList->begin();
            it != gsnL3->pdpInfoList->end();
            it ++)
        {
            if ((*it)->dlTeId == teId &&
                (*it)->ueId == (*ueId))
            {
                break;
            }
        }
    }

    //ERROR_Assert(it != gsnL3->pdpInfoList->end(),
    //             "no PDP link with this NASPI");
    if (it != gsnL3->pdpInfoList->end())
    {
        UmtsLayer3GgsnPdpInfoFinalize(node, gsnL3, *it);
        gsnL3->pdpInfoList->erase(it);
    }
}

// /**
// FUNCTION   :: UmtsLayer3SgsnGtpTunnelData
// LAYER      :: Layer 3
// PURPOSE    :: Handle a GTP msg at SGSN
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + gsnL3     : UmtsLayer3Gsn*   : GSN layer 3 data
// + msg       : Message*         : Message containing the packet
// + teId      : UInt32           : Te Id
// + srcAddr   : Address          : src adrress
// + upLink    : BOOL             : Is it uplink
// RETURN     :: void : NULL
// **/
static
void  UmtsLayer3SgsnGtpTunnelData(Node *node,
                                  UmtsLayer3Data *umtsL3,
                                  UmtsLayer3Gsn* gsnL3,
                                  Message* msg,
                                  UInt32 teId,
                                  Address srcAddr,
                                  BOOL upLink)
{
    UmtsLayer3SgsnFlowInfo* flowInfo = NULL;

    if (DEBUG_GTP_DATA)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_SM);
        printf ("\n\t recv a GTP data pkt teId = %d, uplink = %d.\n",
            teId, upLink);
    }

//GuiStart
    if (node->guiOption == TRUE)
    {
        NodeAddress previousHopNodeId;

        previousHopNodeId = MAPPING_GetNodeIdFromInterfaceAddress(
                                node,
                                srcAddr);
        GUI_Receive(previousHopNodeId, node->nodeId, GUI_NETWORK_LAYER,
                    GUI_DEFAULT_DATA_TYPE,
                    DEFAULT_INTERFACE,
                    DEFAULT_INTERFACE,
                    node->getNodeTime());
    }
//GuiEnd

    // do not need to add GTP header
    // directly send to GGSN/or RNC
    flowInfo = UmtsLayer3GsnFindAppInfoFromList(
                   node,
                   gsnL3,
                   teId,
                   upLink);

    if (flowInfo == NULL)
    {
        // no flow info associated with this tunnel ID, just discard it
        MESSAGE_Free(node, msg);

        // update statistics
        gsnL3->sgsnStat.numTunnelDataPacketDropped ++;

        return;
    }

    if (upLink)
    {
        // from RNC, forward to GGSN
        UmtsLayer3GsnSendGtpMsgOverBackbone(
            node,
            msg,
            flowInfo->ggsnAddr,
            TRUE);

        // update statistics
        gsnL3->sgsnStat.numTunnelDataPacketFwdToGgsn ++;
    }
    else
    {
        // from GGSN, forward to RNC
        UmtsLayer3RncInfo* rncInfo = NULL;
        rncInfo = (UmtsLayer3RncInfo *)
                  FindItem(node,
                           gsnL3->rncList,
                           0,
                           (char *) &(flowInfo->rncId),
                           sizeof(flowInfo->rncId));
        ERROR_Assert(rncInfo, " UMTS: Cannot find RNC info.");
        UmtsAddGtpBackboneHeader(node,
                                 msg,
                                 FALSE,
                                 flowInfo->ueId);

        if (rncInfo->nodeAddr.networkType == NETWORK_IPV4)
        {
            NetworkIpSendRawMessageToMacLayerWithDelay(
                node,
                msg,
                NetworkIpGetInterfaceAddress(
                        node, rncInfo->interfaceIndex),
                GetIPv4Address(rncInfo->nodeAddr),
                IPTOS_PREC_INTERNETCONTROL,
                IPPROTO_CELLULAR,
                1,
                rncInfo->interfaceIndex,
                GetIPv4Address(rncInfo->nodeAddr),
                10 * MILLI_SECOND);
        }
        else if (rncInfo->nodeAddr.networkType == NETWORK_IPV6)
        {
            in6_addr srcAddr;
            Ipv6GetGlobalAggrAddress(node,
                                     rncInfo->interfaceIndex,
                                     &srcAddr);
            Ipv6SendRawMessageToMac(
                node,
                msg,
                srcAddr,
                GetIPv6Address(rncInfo->nodeAddr),
                rncInfo->interfaceIndex,
                IPTOS_PREC_INTERNETCONTROL,
                IPPROTO_CELLULAR,
                1);
        }
        // update statistics
        gsnL3->sgsnStat.numTunnelDataPacketFwdToUe ++;
    }
}

// /**
// FUNCTION   :: UmtsLayer3SgsnGtpHandleCreatePDPContextResponse
// LAYER      :: Layer 3
// PURPOSE    :: Handle a GTP CreatePDPContextResponse msg at SGSN
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + gsnL3     : UmtsLayer3Gsn*   : GSN layer 3 data
// + msg       : Message*         : Message containing the packet
// + srcAddr   : Address          : src adrress
// + gtpHeader : UmtsGtpHeader    : GTP header
// + extHeader : char*            : extended header
// + extHeaderSize : int          : size of the extended header size
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3SgsnGtpHandleCreatePDPContextResponse(
          Node* node,
          UmtsLayer3Data *umtsL3,
          UmtsLayer3Gsn* gsnL3,
          Message* msg,
          Address srcAddr,
          UmtsGtpHeader gtpHeader,
          char* extHeader,
          int extHeaderSize)
{
    UInt32 dlTeId;
    UInt32 ulTeId;
    unsigned char nsapi;
    UmtsGtpCreatePDPContextResponse pdpRspMsg;

    ERROR_Assert(!gsnL3->isGGSN, "NON SGSN rcvd a CREATE PDP CONTEXT RSP");

    if ((DEBUG_GTP || DEBUG_SM))
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_SM);
        printf("\n\t rcvd a CREATE PDP CONTEXT RSP\n");
    }

    // update stas
    gsnL3->sgsnStat.numCreatePDPContextRspRcvd ++;

    memcpy(&pdpRspMsg, MESSAGE_ReturnPacket(msg), sizeof(pdpRspMsg));

    nsapi = pdpRspMsg.nsapi;
    dlTeId = gtpHeader.teId; // DL TEID set by GGSN in the header
    ulTeId = pdpRspMsg.ulTeId;
    UmtsGtpCauseType cause = (UmtsGtpCauseType) pdpRspMsg.gtpCause;

    // to see if the IMSI has been seen
    UmtsLayer3SgsnFlowInfo* appInfo = NULL;

    std::list<UmtsSgsnUeDataInSgsn*>::iterator itUe;
    for (itUe = gsnL3->ueInfoList->begin();
        itUe != gsnL3->ueInfoList->end();
        itUe ++)
    {
        std::list<UmtsLayer3SgsnFlowInfo*>::iterator itApp;
        for (itApp = (*itUe)->appInfoList->begin();
            itApp != (*itUe)->appInfoList->end();
            itApp ++)
        {
            if ((*itApp)->dlTeId == dlTeId)
            {
                appInfo = (*itApp);

                break;
            }
        }
        if (appInfo)
        {
            break;
        }
    }

    //ERROR_Assert(appInfo,
    //    "No AppInfo with this dlTeID in create PDP context Rsp");
    if (!appInfo)
    {
        return;
    }

    NodeId rncId;
    NodeId cellId;
    BOOL found = UmtsLayer3GsnLookupVlr(
                        node,
                        umtsL3,
                        (*itUe)->ueId,
                        &rncId,
                        &cellId);
    ERROR_Assert(found != FALSE, "Cannot find rncId");

    appInfo->ulTeId = ulTeId;
    if ((DEBUG_GTP || DEBUG_SM) && (*itUe)->ueId == DEBUG_UE_ID)
    {
        printf("\n\t rcvd a CREATE PDP CONTEXT RSP for "
            "Ue %d  ulTeId %d dlTeId %d cause %d\n",
            (*itUe)->ueId, appInfo->ulTeId,
            appInfo->dlTeId, cause);
    }
    if (cause == UMTS_GTP_REQUEST_ACCEPTED)
    {
        // Init RAB setup
        UmtsRABServiceInfo ulRabPara;
        UmtsRABServiceInfo dlRabPara;
        UmtsRabConnMap connMap;
        connMap.tunnelMap.ulTeId = ulTeId;
        memcpy(&(connMap.tunnelMap.ggsnAddr),
               &(appInfo->ggsnAddr),
               sizeof(Address));
        connMap.tunnelMap.dlTeId = appInfo->dlTeId;

        // set the QoS
        // assume UL/DL QoS are the same right now
        ulRabPara.guaranteedBitRate = appInfo->qosInfo->ulGuaranteedBitRate;
        ulRabPara.maxBitRate = appInfo->qosInfo->ulMaxBitRate;
        ulRabPara.residualBER = appInfo->qosInfo->residualBER;
        ulRabPara.sduErrorRatio = appInfo->qosInfo->sduErrorRatio;
        ulRabPara.transferDelay = appInfo->qosInfo->transferDelay;
        ulRabPara.trafficClass = appInfo->qosInfo->trafficClass;

        dlRabPara.guaranteedBitRate = appInfo->qosInfo->dlGuaranteedBitRate;
        dlRabPara.maxBitRate = appInfo->qosInfo->dlMaxBitRate;
        dlRabPara.residualBER = appInfo->qosInfo->residualBER;
        dlRabPara.sduErrorRatio = appInfo->qosInfo->sduErrorRatio;
        dlRabPara.transferDelay = appInfo->qosInfo->transferDelay;
        dlRabPara.trafficClass = appInfo->qosInfo->trafficClass;

        UmtsLayer3SgsnSendGtpRabAssgt(
            node,
            umtsL3,
            (*itUe)->ueId,
            &connMap,
            &ulRabPara,
            &dlRabPara,
            rncId);
    }
    else
    {
        // TODO: ACTIVATE PDP REQUEST REJECT
    }
}

// /**
// FUNCTION   :: UmtsLayer3SgsnGtpHandleDeletePDPContextResponse
// LAYER      :: Layer 3
// PURPOSE    :: Handle a GTP CreatePDPContextResponse msg at SGSN
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + gsnL3     : UmtsLayer3Gsn*   : GSN layer 3 data
// + msg       : Message*         : Message containing the packet
// + srcAddr   : Address          : src adrress
// + gtpHeader : UmtsGtpHeader    : GTP header
// + extHeader : char*            : extended header
// + extHeaderSize : int          : size of the extended header size
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3SgsnGtpHandleDeletePDPContextResponse(
          Node* node,
          UmtsLayer3Data *umtsL3,
          UmtsLayer3Gsn* gsnL3,
          Message* msg,
          Address srcAddr,
          UmtsGtpHeader gtpHeader,
          char* extHeader,
          int extHeaderSize)
{
    UmtsLayer3SgsnFlowInfo* flowInfo = NULL;
    UInt32 dlTeId;
    //unsigned char nsapi;
    NodeId rncId;
    NodeId cellId;

    ERROR_Assert(!gsnL3->isGGSN, "NON SGSN rcvd a DELETE PDP CONTEXT RSP");

    // update statistics
    gsnL3->sgsnStat.numDeletePDPContextRspRcvd ++;

    if ((DEBUG_GTP || DEBUG_SM))
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_SM);
        printf("\n\t rcvd a DELETE PDP CONTEXT RSP\n");
    }

    UmtsGtpDeletePDPContextResponse pdpRspMsg;
    memcpy(&pdpRspMsg, MESSAGE_ReturnPacket(msg), sizeof(pdpRspMsg));

    dlTeId = gtpHeader.teId; // DL TEID put by GGSN in the header
    UmtsGtpCauseType casue = (UmtsGtpCauseType) pdpRspMsg.gtpCause;

    // find out the session info
    std::list<UmtsSgsnUeDataInSgsn*>::iterator itUe;
    for (itUe = gsnL3->ueInfoList->begin();
        itUe != gsnL3->ueInfoList->end();
        itUe ++)
    {
        std::list<UmtsLayer3SgsnFlowInfo*>::iterator itApp;
        for (itApp = (*itUe)->appInfoList->begin();
            itApp != (*itUe)->appInfoList->end();
            itApp ++)
        {
            if ((*itApp)->dlTeId == dlTeId)
            {
                flowInfo = (*itApp);

                break;
            }
        }
        if (flowInfo)
        {
            break;
        }
    }
    //ERROR_Assert(flowInfo, "No flow w/ "
    //             "the dlTeID in delete PDP context Rsp");
    if (!flowInfo)
    {
        return;
    }

    BOOL found = UmtsLayer3GsnLookupVlr(node,
                                        umtsL3,
                                        flowInfo->ueId,
                                        &rncId,
                                        &cellId);
    ERROR_Assert(found != FALSE, "Cannot find UE in my VLR!");

    if (casue == UMTS_GTP_REQUEST_ACCEPTED)
    {
        UmtsLayer3GsnSmSendDeactivatePDPContextAccept(
            node, umtsL3, gsnL3,
            flowInfo->srcDestType, flowInfo->transId, flowInfo->ueId);

        // update stat
        gsnL3->sgsnStat.numDeactivatePDPContextAcptSent ++;

        if (flowInfo->cmData.smData->smState ==
            UMTS_SM_NW_PDP_INACTIVE_PENDING)
        {
            // if UE init deactivation and NW init DEACTIVATIOn collide
            // cancel T3395
            if (flowInfo->cmData.smData->T3395)
            {
                MESSAGE_CancelSelfMsg(node, flowInfo->cmData.smData->T3395);
                flowInfo->cmData.smData->T3395 = NULL;

            }
        }
        // move state to inactive
        flowInfo->cmData.smData->smState = UMTS_SM_NW_PDP_INACTIVE;

        //Tell the RNC to release RAB
        UmtsLayer3SgsnSendGtpRabRelease(
            node,
            umtsL3,
            flowInfo->ueId,
            flowInfo->rabId,
            rncId);

        UmtsLayer3GsnRemoveAppInfoFromList(
            node, gsnL3, (*itUe)->transIdBitSet, (*itUe)->appInfoList,
            flowInfo->srcDestType, flowInfo->transId);
    }
    else
    {
        // do nothing
    }
}

// /**
// FUNCTION   :: UmtsLayer3SgsnGtpHandlePduNotificationRequest
// LAYER      :: Layer 3
// PURPOSE    :: Handle a GTP PDU Notification Request msg at SGSN
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + gsnL3     : UmtsLayer3Gsn*   : GSN layer 3 data
// + msg       : Message*         : Message containing the packet
// + srcAddr   : Address          : src adrress
// + gtpHeader : UmtsGtpHeader    : GTP header
// + extHeader : char*            : extended header
// + extHeaderSize : int          : size of the extended header size
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3SgsnGtpHandlePduNotificationRequest(
          Node* node,
          UmtsLayer3Data* umtsL3,
          UmtsLayer3Gsn* gsnL3,
          Message* msg,
          Address srcAddr,
          UmtsGtpHeader gtpHeader,
          char* extHeader,
          int extHeaderSize)
{
    UmtsGtpPduNotificationRequest pduReqMsg;
    UmtsLayer3SgsnFlowInfo* flowInfo = NULL;
    UmtsSgsnUeDataInSgsn* ueInfo = NULL;
    UInt32 ulTeId;
    UInt32 ueId;
    NodeId rncId;
    NodeId cellId;
    int index = 0;
    BOOL found = FALSE;
    BOOL isSucc = TRUE;
    char* pduPacket;

    // update stats
    gsnL3->sgsnStat.numPduNotificationReqRcvd ++;

    memcpy(&pduReqMsg, MESSAGE_ReturnPacket(msg), sizeof(pduReqMsg));

    ulTeId = gtpHeader.teId; // UL TEID put by GGSN in the header
    ueId = pduReqMsg.ueId;

    if ((DEBUG_GTP || DEBUG_SM) && ueId == DEBUG_UE_ID)
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_SM);
        printf("\n\t rcvd a PDU Notification Request\n");
    }

    // check if the flow has been established
    std::list<UmtsSgsnUeDataInSgsn*>::iterator itUe;
    for (itUe = gsnL3->ueInfoList->begin();
         itUe != gsnL3->ueInfoList->end();
         itUe ++)
    {
        if ((*itUe)->ueId == ueId)
        {
            // found the ue information
            ueInfo = (*itUe);
            break;
        }
    }

    if (ueInfo != NULL)
    {
        // check if the flow already exist
        std::list<UmtsLayer3SgsnFlowInfo*>::iterator itFlow;
        for (itFlow = (*itUe)->appInfoList->begin();
            itFlow != (*itUe)->appInfoList->end();
            itFlow ++)
        {
            if ((*itFlow)->ulTeId == ulTeId)
            {
                flowInfo = (*itFlow);
                break;
            }
        }

        if (flowInfo == NULL)
        {
            // new flow, add it into list
            int pos;
            unsigned char transId;

            // allocate transaction ID
            pos = UmtsGetPosForNextZeroBit<UMTS_MAX_TRANS_MOBILE>
                                        (*(ueInfo->transIdBitSet), 0);
            if (pos == -1)
            {
                // too many pending flows
                isSucc = FALSE;
            }
            else
            {
                UmtsLayer3FlowClassifier flowClsInfo;

                NodeAddress ipv4Addr; // pdp dest's addr
                //NodeId pdpNodeId;

                transId = UMTS_LAYER3_NETWORK_MOBILE_START +
                          (unsigned char) pos;
                ueInfo->transIdBitSet->set(pos);
                flowClsInfo.domainInfo = UMTS_LAYER3_CN_DOMAIN_PS;
                flowInfo = UmtsLayer3GsnAddAppInfoToList(
                               node,
                               gsnL3,
                               ueInfo->appInfoList,
                               &flowClsInfo,
                               UMTS_FLOW_NETWORK_ORIGINATED,
                               transId,
                               0);

                flowInfo->ulTeId = ulTeId;
                flowInfo->ueId = ueId;
                flowInfo->dlTeId = gsnL3->teIdCount ++;

                // TODO: ggsn addrerss should be included inthe msg

                found = UmtsLayer3GsnLookupVlr(
                    node, umtsL3, ueId, &rncId, &cellId);
                ERROR_Assert(found, "UMTS (SGSN): Unable to find UE info!");

                flowInfo->rncId = rncId;

                // get QoS  info
                index += sizeof(UmtsGtpPduNotificationRequest);

                pduPacket = (char*)MESSAGE_ReturnPacket(msg);
                memcpy((char*)&ipv4Addr,
                        &pduPacket[index],
                        pduReqMsg.pdpAddrLen);
                //pdpNodeId = MAPPING_GetNodeIdFromInterfaceAddress(
                //                       node, ipv4Addr);
                //if (ueId == pdpNodeId)

                // get the QoS info
                // traffic class
                UmtsLayer3ConvertOctetToTrafficClass(
                    pduReqMsg.trafficClass,
                    pduReqMsg.trafficHandlingPrio,
                    &(flowInfo->qosInfo->trafficClass));

                // mx bit rate
                UmtsLayer3ConvertOctetToMaxRate(
                    pduReqMsg.ulMaxBitRate,
                    &(flowInfo->qosInfo->ulMaxBitRate));
                UmtsLayer3ConvertOctetToMaxRate(
                    pduReqMsg.dlMaxBitRate,
                    &(flowInfo->qosInfo->dlMaxBitRate));

                UmtsLayer3ConvertOctetToMaxRate(
                    pduReqMsg.ulGuaranteedBitRate,
                    &(flowInfo->qosInfo->ulGuaranteedBitRate));
                UmtsLayer3ConvertOctetToMaxRate(
                    pduReqMsg.dlGuaranteedBitRate,
                    &(flowInfo->qosInfo->dlGuaranteedBitRate));


                // delay
                /*if (flowInfo->qosInfo->trafficClass <
                    UMTS_QOS_CLASS_INTERACTIVE_3)*/
                {
                    UmtsLayer3ConvertOctetToDelay(
                        pduReqMsg.transferDelay,
                        &(flowInfo->qosInfo->transferDelay));
                }
                if (DEBUG_QOS)
                {
                    char clkStr[MAX_STRING_LENGTH];

                    if (flowInfo->classifierInfo->domainInfo ==
                        UMTS_LAYER3_CN_DOMAIN_CS)
                    {
                        UmtsLayer3PrintRunTimeInfo(
                            node, UMTS_LAYER3_SUBLAYER_CC);
                    }
                    else if (flowInfo->classifierInfo->domainInfo ==
                             UMTS_LAYER3_CN_DOMAIN_PS)
                    {
                        UmtsLayer3PrintRunTimeInfo(
                            node, UMTS_LAYER3_SUBLAYER_SM);

                    }
                    TIME_PrintClockInSecond(
                        flowInfo->qosInfo->transferDelay, clkStr);

                    printf("\n\t handle PDU notification "
                        "req for a new flow\n");
                    printf("\n\t -------QoS Info------");
                    printf("\n\t trafficClass %d \t UL maxBitRate %d"
                         "\t UL guaranteedBitRate %d",
                         flowInfo->qosInfo->trafficClass,
                         flowInfo->qosInfo->ulMaxBitRate,
                         flowInfo->qosInfo->ulGuaranteedBitRate);
                    printf("\n\t DL maxBitRate %d DL "
                        "guaranteedBitRate %d \n"
                         "\t sduErrorRatio %f \t residualBER %f",
                         flowInfo->qosInfo->dlMaxBitRate,
                         flowInfo->qosInfo->dlGuaranteedBitRate,
                         (double)flowInfo->qosInfo->sduErrorRatio,
                         flowInfo->qosInfo->residualBER);
                   printf(" \t transferDelay %s\n",
                         clkStr);
                }

                if (ueInfo->gmmData->pmmState != UMTS_PMM_CONNECTED)
                {
                    UmtsPagingCause cause;
                    if (flowInfo->qosInfo->trafficClass ==
                        UMTS_QOS_CLASS_CONVERSATIONAL)
                    {
                        cause = UMTS_PAGING_CAUSE_CONVERSATION_CALL;
                    }
                    else if (flowInfo->qosInfo->trafficClass ==
                        UMTS_QOS_CLASS_STREAMING)
                    {
                        cause = UMTS_PAGING_CAUSE_STREAMING_CALL;
                    }
                    else if (flowInfo->qosInfo->trafficClass >
                        UMTS_QOS_CLASS_STREAMING &&
                        flowInfo->qosInfo->trafficClass <
                        UMTS_QOS_CLASS_BACKGROUND)
                    {
                        cause = UMTS_PAGING_CAUSE_INTERACTIVE_CALL;
                    }
                    else if (flowInfo->qosInfo->trafficClass ==
                        UMTS_QOS_CLASS_BACKGROUND)
                    {
                        cause = UMTS_PAGING_CAUSE_BACKGROUND_CALL;
                    }
                    else
                    {
                        cause = UMTS_PAGING_CAUSE_UNKNOWN;
                    }

                    // set state to wait for paging results
                    flowInfo->cmData.smData->smState =
                        UMTS_SM_NW_PAGE_PENDING;

                    if (ueInfo->gmmData->T3313 == NULL)
                    {
                        // page the UE first
                        UmtsLayer3SgsnSendPaging(
                            node,
                            umtsL3,
                            ueId,
                            UMTS_LAYER3_CN_DOMAIN_PS,
                            cause);

                        //Init paging timer
                        char timerInfo[30];
                        int index = 0;
                        memcpy(&timerInfo[index], &ueId, sizeof(ueId));
                        index += sizeof(ueId);
                        timerInfo[index ++] = (char)cause;
                        ueInfo->gmmData->T3313 =
                                        UmtsLayer3SetTimer(
                                            node,
                                            umtsL3,
                                            UMTS_LAYER3_TIMER_T3313,
                                            UMTS_LAYER3_TIMER_T3313_VAL,
                                            NULL,
                                            (void*)&timerInfo[0],
                                            index);
                        ueInfo->gmmData->counterT3313 = 0;
                    }
                }
                else
                {
                    // already connected, send Request PDP Activation to UE
                    flowInfo->cmData.smData->smState =
                        UMTS_SM_NW_PDP_INACTIVE;
                    UmtsLayer3RequestPDPContextActivation reqPdpMsg;

                    // fill the contents
                    memset(&reqPdpMsg, 0, sizeof(reqPdpMsg));
                    reqPdpMsg.trafficClass =
                        (unsigned char)flowInfo->qosInfo->trafficClass;

                    UmtsLayer3GsnSendNasMsg(
                        node,
                        umtsL3,
                        (char*)&reqPdpMsg,
                        sizeof(reqPdpMsg),
                        (char)UMTS_LAYER3_PD_SM,
                        (char)UMTS_SM_MSG_TYPE_REQUEST_PDP_ACTIVATION,
                        flowInfo->transId,
                        ueId);

                    // update statistics
                    gsnL3->sgsnStat.numReqActivatePDPContextSent ++;
                }
            }
        }
    }
    else
    {
        isSucc = FALSE;
    }
}

// /**
// FUNCTION   :: UmtsLayer3GgsnGtpHandleTunneledData
// LAYER      :: Layer 3
// PURPOSE    :: Handle a GTP CreatePDPContextRequest msg at GGSN
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + gsnL3     : UmtsLayer3Gsn*   : GSN layer 3 data
// + msg       : Message*         : Message containing the packet
// + srcAddr   : Address          : src adrress
// + gtpHeader : UmtsGtpHeader    : GTP header
// + srcAddr   : Address          : Source node address
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3GgsnGtpHandleTunneledData(
          Node* node,
          UmtsLayer3Data *umtsL3,
          UmtsLayer3Gsn* gsnL3,
          Message* msg,
          UmtsGtpHeader gtpHeader,
          Address srcAddr)
{
    UInt32 ulTeId;
    ulTeId = gtpHeader.teId;

    UmtsLayer3GgsnPdpInfo* pdpInfo;
    pdpInfo = UmtsLayer3GgsnFindPdpInfoByTeId(
                  node, umtsL3, gsnL3, ulTeId, NULL, TRUE);

    if (DEBUG_ASSERT)
    {
        ERROR_Assert(pdpInfo != NULL, "No PDP link with UL teId");
    }
    if (!pdpInfo)
    {
        MESSAGE_Free(node, msg);
        return;
    }

    if ((DEBUG_GTP_DATA))
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_SM);
        printf("\n\t rcvd a tunneled data\n");
        UmtsPrintMessage(std::cout, msg);
    }
    // Get the classsfier info
    const char* payload;
    UmtsLayer3FlowClassifier appClsInfo;
    TosType tos = 0;
    int interfaceIndex = 0; // not used

    //UmtsLayer3FlowClassifier* appClsPt = NULL;
    // 1. build the callisifer from the msg

    // retrive information from the packet headers
    payload = MESSAGE_ReturnPacket(msg);
    memset(&appClsInfo, 0, sizeof(UmtsLayer3FlowClassifier));

    // build the classifier info struct
    UmtsLayer3BuildFlowClassifierInfo(
            node,
            interfaceIndex,
            NETWORK_IPV4, //networkType,
            msg,
            &tos,
            &appClsInfo,
            &payload);
    // assume IPv4 right now for testing
    interfaceIndex = NetworkGetInterfaceIndexForDestAddress(
                            node,
                            GetIPv4Address(appClsInfo.dstAddr));
    if (DEBUG_GTP && 0)
    {
        char addrStr[MAX_STRING_LENGTH];
        IO_ConvertIpAddressToString(&appClsInfo.dstAddr, addrStr);
        printf("Rout message to %s via interface: %d\n",
               addrStr, interfaceIndex);
    }

//GuiStart
    if (node->guiOption == TRUE)
    {
        NodeAddress previousHopNodeId;

        previousHopNodeId = MAPPING_GetNodeIdFromInterfaceAddress(
                                node,
                                srcAddr);
        GUI_Receive(previousHopNodeId, node->nodeId, GUI_NETWORK_LAYER,
                    GUI_DEFAULT_DATA_TYPE,
                    DEFAULT_INTERFACE,
                    DEFAULT_INTERFACE,
                    node->getNodeTime());
    }
//GuiEnd

    // update statistics
    gsnL3->ggsnStat.numPsDataPacketFromPlmn ++;

    // check packet size or other QoS paramenters
    if (!UmtsLayer3CheckFlowQoSParam(
            node, interfaceIndex,
            (TraceProtocolType) msg->originatingProtocol,
            tos, &payload))
    {
        MESSAGE_Free(node, msg);

        gsnL3->ggsnStat.numPsDataPacketDropped ++;
        gsnL3->ggsnStat.numPsDataPacketDroppedUnsupportedFormat ++;

        return;
    }

    if (UmtsLayer3GgsnIsPacketForMyPlmn(node, msg, 0, NETWORK_IPV4))
    {
        //Set message protocol as CELLULAR so
        //it can not mistaken as IP packet
        //within cellular network
        MESSAGE_SetLayer(msg, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR);
        // destination in my network too
        UmtsLayer3GgsnHandlePacketFromUpperOrOutside(
            node,
            msg,
            0,
            NETWORK_IPV4,
            TRUE);
    }
    else
    {
        //Set message protocol as IP so it can
        //not mistaken as cellular packet
        //by external IP node, assuming only IPv4 is supported.
        MESSAGE_SetLayer(msg, NETWORK_LAYER, NETWORK_PROTOCOL_IP);
        RoutePacketAndSendToMac(node,
                                msg,
                                CPU_INTERFACE,
                                interfaceIndex,
                                ANY_IP);

        // update statistics
        gsnL3->ggsnStat.numPsDataPacketToOutsidePlmn ++;

        // update dynamic stat
        if (node->guiOption)
        {
            UmtsMeasElement measElem;
            measElem.measVal = MESSAGE_ReturnPacketSize(msg) *8;
            measElem.timeStamp = node->getNodeTime();

            // put to the meas Lsit
            gsnL3->ggsnStat.egressThruputMeas->AddMeasElement(
                     measElem, node->getNodeTime());
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3GgsnGtpHandleCreatePDPContextRequest
// LAYER      :: Layer 3
// PURPOSE    :: Handle a GTP CreatePDPContextRequest msg at GGSN
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + gsnL3     : UmtsLayer3Gsn*   : GSN layer 3 data
// + msg       : Message*         : Message containing the packet
// + srcAddr   : Address          : Source node address
// + gtpHeader : UmtsGtpHeader    : GTP header
// + extHeader : char*            : extended header
// + extheaderSize : int          : size of the extended header
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3GgsnGtpHandleCreatePDPContextRequest(
          Node* node,
          UmtsLayer3Data *umtsL3,
          UmtsLayer3Gsn* gsnL3,
          Message* msg,
          Address srcAddr,
          UmtsGtpHeader gtpHeader,
          char* extHeader,
          int extHeaderSize)
{
    UInt32 dlTeId;
    UInt32 ulTeId;
    unsigned char nsapi;
    UmtsLayer3GgsnPdpInfo* pdpInfo;

    ERROR_Assert(gsnL3->isGGSN, "NON GGSN rcvd a CREATE PDP CONTEXT REQ");

    if ((DEBUG_GTP || DEBUG_SM))
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_SM);
        printf("\n\t rcvd a CREATE PDP CONTEXT REQ\n");
    }
    UmtsGtpCreatePDPContextRequest pdpReqMsg;
    memcpy(&pdpReqMsg, MESSAGE_ReturnPacket(msg), sizeof(pdpReqMsg));

    // update statistics
    gsnL3->ggsnStat.numCreatePDPContextReqRcvd ++;

    dlTeId = pdpReqMsg.dlTeId; // the same as gtpHeader.teId;
    nsapi = pdpReqMsg.nsapi;

    ulTeId = gtpHeader.teId;
    pdpInfo = UmtsLayer3GgsnFindPdpInfoByTeId(
                  node, umtsL3, gsnL3,
                  ulTeId, NULL, TRUE);

    if (pdpInfo == NULL)
    {
        // generate ul TEID
        ulTeId = gsnL3->teIdCount ++;
        pdpInfo = UmtsLayer3GgsnAddPdpInfoToList(
                      node, umtsL3, gsnL3,
                      &nsapi, &dlTeId, &ulTeId, NULL);
        pdpInfo->qosInfo->trafficClass =
            (UmtsQoSTrafficClass)pdpReqMsg.trafficClass;
        pdpInfo->srcDestType = UMTS_FLOW_MOBILE_ORIGINATED;
        pdpInfo->sgsnAddr = srcAddr;
        pdpInfo->sgsnId =
            MAPPING_GetNodeIdFromInterfaceAddress(node, srcAddr);

        gsnL3->ggsnStat.numMoFlowRequested ++;
        gsnL3->ggsnStat.numMoFlowAdmitted ++;
        if (pdpInfo->qosInfo->trafficClass == UMTS_QOS_CLASS_CONVERSATIONAL)
        {
            gsnL3->ggsnStat.numMoConversationalFlowRequested ++;
            gsnL3->ggsnStat.numMoConversationalFlowAdmitted ++;
        }
        else if (pdpInfo->qosInfo->trafficClass == UMTS_QOS_CLASS_STREAMING)
        {
            gsnL3->ggsnStat.numMoStreamingFlowRequested ++;
            gsnL3->ggsnStat.numMoStreamingFlowAdmitted ++;
        }
        else if (pdpInfo->qosInfo->trafficClass ==
                 UMTS_QOS_CLASS_BACKGROUND)
        {
            gsnL3->ggsnStat.numMoBackgroundFlowRequested ++;
            gsnL3->ggsnStat.numMoBackgroundFlowAdmitted ++;
        }
        else
        {
            gsnL3->ggsnStat.numMoInteractiveFlowRequested ++;
            gsnL3->ggsnStat.numMoInteractiveFlowAdmitted ++;
        }
    }
    else
    {
        pdpInfo->nsapi = nsapi;
        pdpInfo->dlTeId = dlTeId;
        pdpInfo->sgsnAddr = srcAddr;
        pdpInfo->sgsnId =
            MAPPING_GetNodeIdFromInterfaceAddress(node, srcAddr);
    }

    UmtsGtpCauseType cause = UMTS_GTP_REQUEST_ACCEPTED;

    // Send CREATE PDP CONTEXT REPSONSE
    UmtsLayer3GsnGtpSendCreatePDPContextResponse(node, umtsL3, gsnL3,
        pdpInfo, cause);
}

// /**
// FUNCTION   :: UmtsLayer3GsnGtpHandleDeletePDPContextRequest
// LAYER      :: Layer 3
// PURPOSE    :: Handle a GTP CreatePDPContextRequest msg at GGSN
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + gsnL3     : UmtsLayer3Gsn*   : GSN layer 3 data
// + msg       : Message*         : Message containing the packet
// + srcAddr   : Address          : Source node address
// + gtpHeader : UmtsGtpHeader    : GTP header
// + extHeader : char*            : extended header
// + extheaderSize : int          : size of the extended header
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3GsnGtpHandleDeletePDPContextRequest(
          Node* node,
          UmtsLayer3Data *umtsL3,
          UmtsLayer3Gsn* gsnL3,
          Message* msg,
          Address srcAddr,
          UmtsGtpHeader gtpHeader,
          char* extHeader,
          int extHeaderSize)
{
    UInt32 teId;
    unsigned char nsapi;

    if ((DEBUG_GTP || DEBUG_SM))
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_SM);
        printf("\n\t rcvd a DELETE PDP CONTEXT REQ\n");
    }
    UmtsGtpDeletePDPContextRequest pdpReqMsg;
    memcpy(&pdpReqMsg, MESSAGE_ReturnPacket(msg), sizeof(pdpReqMsg));

    nsapi = pdpReqMsg.nsapi;

    // get ul TEID
    teId = gtpHeader.teId;

    UmtsGtpCauseType cause = UMTS_GTP_REQUEST_ACCEPTED;
    if (gsnL3->isGGSN)
    {
        UmtsLayer3GgsnPdpInfo* pdpInfo;

        // update statistics
        gsnL3->ggsnStat.numDeletePDPContextReqRcvd ++;

        // UE initiated PDP Context Deactivation
        pdpInfo = UmtsLayer3GgsnFindPdpInfoByTeId(
                      node, umtsL3, gsnL3,
                      teId, NULL, TRUE);
        if (DEBUG_ASSERT)
        {
            ERROR_Assert(pdpInfo, "No PDP info link with this ul TEID");
        }

        if (!pdpInfo)
        {
            return;
        }

        // Send DELETE PDP CONTEXT REPSONSE
        UmtsLayer3GsnGtpSendDeletePDPContextResponse(
            node, umtsL3, gsnL3, pdpInfo->nsapi,
            (unsigned char)pdpInfo->ulTeId,
            (unsigned char)pdpInfo->dlTeId,
            cause, pdpInfo->sgsnAddr);

        // update flow related statistics
        if (pdpInfo->srcDestType == UMTS_FLOW_NETWORK_ORIGINATED)
        {
            gsnL3->ggsnStat.numMtFlowCompleted ++;
        }
        else
        {
            gsnL3->ggsnStat.numMoFlowCompleted ++;
        }
        if (pdpInfo->qosInfo->trafficClass == UMTS_QOS_CLASS_CONVERSATIONAL)
        {
            if (pdpInfo->srcDestType == UMTS_FLOW_NETWORK_ORIGINATED)
            {
                gsnL3->ggsnStat.numMtConversationalFlowCompleted ++;
            }
            else
            {
                gsnL3->ggsnStat.numMoConversationalFlowCompleted ++;
            }
        }
        else if (pdpInfo->qosInfo->trafficClass == UMTS_QOS_CLASS_STREAMING)
        {
            if (pdpInfo->srcDestType == UMTS_FLOW_NETWORK_ORIGINATED)
            {
                gsnL3->ggsnStat.numMtStreamingFlowCompleted ++;
            }
            else
            {
                gsnL3->ggsnStat.numMoStreamingFlowCompleted ++;
            }
        }
        else if (pdpInfo->qosInfo->trafficClass ==
            UMTS_QOS_CLASS_BACKGROUND)
        {
            if (pdpInfo->srcDestType == UMTS_FLOW_NETWORK_ORIGINATED)
            {
                gsnL3->ggsnStat.numMtBackgroundFlowCompleted ++;
            }
            else
            {
                gsnL3->ggsnStat.numMoBackgroundFlowCompleted ++;
            }
        }
        else
        {
            if (pdpInfo->srcDestType == UMTS_FLOW_NETWORK_ORIGINATED)
            {
                gsnL3->ggsnStat.numMtInteractiveFlowCompleted ++;
            }
            else
            {
                gsnL3->ggsnStat.numMoInteractiveFlowCompleted ++;
            }
        }

        // remove and free the data structure
        UmtsLayer3GgsnRemovePdpInfoByTeId(
            node, umtsL3, gsnL3, teId, NULL, TRUE);
    }
    else
    {
        UmtsLayer3SgsnFlowInfo* flowInfo;

        // update statistics
        gsnL3->sgsnStat.numDeletePDPContextReqRcvd ++;

        flowInfo = UmtsLayer3GsnFindAppInfoFromList(
                       node,
                       gsnL3,
                       teId,
                       FALSE);

        if (DEBUG_ASSERT)
        {
            ERROR_Assert(flowInfo, "No flow info linked with this dl TEID");
        }
        if (!flowInfo)
        {
            return;
        }

        // Send Deactivate PDP Context Request to UE
        if (!flowInfo->cmData.smData->T3395)
        {
            UmtsLayer3GsnSmSendDeactivatePDPContextRequest(
                node,
                umtsL3,
                gsnL3,
                flowInfo,
                UMTS_SM_CAUSE_REGUALR_DEACTIVATION);
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3GgsnGtpHandleDeletePDPContextResponse
// LAYER      :: Layer 3
// PURPOSE    :: Handle a GTP CreatePDPContextResponse msg at GGSN
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + gsnL3     : UmtsLayer3Gsn*   : GSN layer 3 data
// + msg       : Message*         : Message containing the packet
// + srcAddr   : Address          : Source node address
// + gtpHeader : UmtsGtpHeader    : GTP header
// + extHeader : char*            : extended header
// + extheaderSize : int          : size of the extended header
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3GgsnGtpHandleDeletePDPContextResponse(
          Node* node,
          UmtsLayer3Data *umtsL3,
          UmtsLayer3Gsn* gsnL3,
          Message* msg,
          Address srcAddr,
          UmtsGtpHeader gtpHeader,
          char* extHeader,
          int extHeaderSize)
{
    UmtsGtpDeletePDPContextResponse pdpRsp;
    UmtsGtpDeletePDPContextResponse *pdpRspMsg = &pdpRsp;
    UmtsLayer3GgsnPdpInfo* pdpInfo;
    UmtsGtpCauseType cause;
    UInt32 teId;

    // update statistics
    gsnL3->ggsnStat.numDeletePDPContextRspRcvd ++;

    memcpy(pdpRspMsg,
           MESSAGE_ReturnPacket(msg),
           sizeof(UmtsGtpDeletePDPContextResponse));
    cause = (UmtsGtpCauseType) pdpRspMsg->gtpCause;

    if ((DEBUG_GTP || DEBUG_SM))
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_SM);
        printf("\n\t rcvd a DELETE PDP CONTEXT RES\n");
    }

    // get ul TEID
    teId = gtpHeader.teId;

    // update flow related statistics
    pdpInfo = UmtsLayer3GgsnFindPdpInfoByTeId(
                  node, umtsL3, gsnL3,
                  teId, NULL, TRUE);
    //ERROR_Assert(pdpInfo, "No PDP info link with this ul TEID");
    if (!pdpInfo)
    {
        return;
    }

    if (pdpInfo->srcDestType == UMTS_FLOW_NETWORK_ORIGINATED)
    {
        gsnL3->ggsnStat.numMtFlowCompleted ++;
    }
    else
    {
        gsnL3->ggsnStat.numMoFlowCompleted ++;
    }
    if (pdpInfo->qosInfo->trafficClass == UMTS_QOS_CLASS_CONVERSATIONAL)
    {
        if (pdpInfo->srcDestType == UMTS_FLOW_NETWORK_ORIGINATED)
        {
            gsnL3->ggsnStat.numMtConversationalFlowCompleted ++;
        }
        else
        {
            gsnL3->ggsnStat.numMoConversationalFlowCompleted ++;
        }
    }
    else if (pdpInfo->qosInfo->trafficClass == UMTS_QOS_CLASS_STREAMING)
    {
        if (pdpInfo->srcDestType == UMTS_FLOW_NETWORK_ORIGINATED)
        {
            gsnL3->ggsnStat.numMtStreamingFlowCompleted ++;
        }
        else
        {
            gsnL3->ggsnStat.numMoStreamingFlowCompleted ++;
        }
    }
    else if (pdpInfo->qosInfo->trafficClass == UMTS_QOS_CLASS_BACKGROUND)
    {
        if (pdpInfo->srcDestType == UMTS_FLOW_NETWORK_ORIGINATED)
        {
            gsnL3->ggsnStat.numMtBackgroundFlowCompleted ++;
        }
        else
        {
            gsnL3->ggsnStat.numMoBackgroundFlowCompleted ++;
        }
    }
    else
    {
        if (pdpInfo->srcDestType == UMTS_FLOW_NETWORK_ORIGINATED)
        {
            gsnL3->ggsnStat.numMtInteractiveFlowCompleted ++;
        }
        else
        {
            gsnL3->ggsnStat.numMoInteractiveFlowCompleted ++;
        }
    }

    // remove from list
    UmtsLayer3GgsnRemovePdpInfoByTeId(
        node, umtsL3, gsnL3, teId, NULL, TRUE);
}


// /**
// FUNCTION   :: UmtsLayer3GgsnGtpHandlePduNotificationResponse
// LAYER      :: Layer 3
// PURPOSE    :: Handle a PDU Notification Response msg at GGSN
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + gsnL3     : UmtsLayer3Gsn*   : GSN layer 3 data
// + msg       : Message*         : Message containing the packet
// + srcAddr   : Address          : From which the msg come
// + gtpHeader : UmtsGtpHeader    : GTP header
// + extHeader : char*            : Extended header
// + extHeaderSize : int          : Size of extended header
// RETURN     :: void             : NULL
// **/
static
void UmtsLayer3GgsnGtpHandlePduNotificationResponse(
          Node* node,
          UmtsLayer3Data *umtsL3,
          UmtsLayer3Gsn* gsnL3,
          Message* msg,
          Address srcAddr,
          UmtsGtpHeader gtpHeader,
          char* extHeader,
          int extHeaderSize)
{
    UmtsGtpPduNotificationResponse pduRepMsg;
    UmtsLayer3GgsnPdpInfo* pdpInfo;
    UInt32 ulTeId;

    ERROR_Assert(gsnL3->isGGSN, "NON GGSN rcvd a CREATE PDP CONTEXT REQ");

    if ((DEBUG_GTP || DEBUG_SM))
    {
        UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_SM);
        printf("\n\t rcvd a PDU Notification Response with UL Te ID %d\n",
               gtpHeader.teId);
    }

    // update statistics
    gsnL3->ggsnStat.numPduNotificationRspRcvd ++;

    memcpy(&pduRepMsg, MESSAGE_ReturnPacket(msg), sizeof(pduRepMsg));

    ulTeId = gtpHeader.teId;
    pdpInfo = UmtsLayer3GgsnFindPdpInfoByTeId(
                  node, umtsL3, gsnL3,
                  ulTeId, NULL, TRUE);

    if (pdpInfo == NULL)
    {
        // For unknown PDP context, just ignore it
        return;
    }

    if (pduRepMsg.gtpCause == UMTS_GTP_REQUEST_ACCEPTED)
    {
        // accepted, just start forwarding packet
        pdpInfo->status = UMTS_GGSN_PDP_ACTIVE;

        gsnL3->ggsnStat.numMtFlowAdmitted ++;
        if (pdpInfo->qosInfo->trafficClass == UMTS_QOS_CLASS_CONVERSATIONAL)
        {
            gsnL3->ggsnStat.numMtConversationalFlowAdmitted ++;
        }
        else if (pdpInfo->qosInfo->trafficClass == UMTS_QOS_CLASS_STREAMING)
        {
            gsnL3->ggsnStat.numMtStreamingFlowAdmitted ++;
        }
        else if (pdpInfo->qosInfo->trafficClass ==
            UMTS_QOS_CLASS_BACKGROUND)
        {
            gsnL3->ggsnStat.numMtBackgroundFlowAdmitted ++;
        }
        else
        {
            gsnL3->ggsnStat.numMtInteractiveFlowAdmitted ++;
        }

        // send out queued packets
        if (pdpInfo->srcDestType == UMTS_FLOW_NETWORK_ORIGINATED)
        {
            Message* msgInBuffer = NULL;

            while (!pdpInfo->inPacketBuf->empty())
            {
                msgInBuffer = pdpInfo->inPacketBuf->front();
                pdpInfo->inPacketBuf->pop();
                pdpInfo->pktBufSize
                    -= MESSAGE_ReturnPacketSize(msgInBuffer);
                gsnL3->agregPktBufSize
                    -= MESSAGE_ReturnPacketSize(msgInBuffer);

                UmtsLayer3GgsnGtpTunnelData(
                    node,
                    umtsL3,
                    gsnL3,
                    pdpInfo,
                    msgInBuffer);
            }
        }

        pdpInfo->lastActiveTime = node->getNodeTime();

    }
    else if (pduRepMsg.gtpCause == UMTS_GTP_REQUEST_REJECTED)
    {
        // failed, marked it as to be deleted
        pdpInfo->status = UMTS_GGSN_PDP_REJECTED;
        pdpInfo->lastActiveTime = node->getNodeTime();

        if (pdpInfo->srcDestType == UMTS_FLOW_NETWORK_ORIGINATED)
        {
            // update statistics
            gsnL3->ggsnStat.numMtFlowRejected ++;
            if (pdpInfo->qosInfo->trafficClass ==
                UMTS_QOS_CLASS_CONVERSATIONAL)
            {
                gsnL3->ggsnStat.numMtConversationalFlowRejected ++;
            }
            else if (pdpInfo->qosInfo->trafficClass ==
                UMTS_QOS_CLASS_STREAMING)
            {
                gsnL3->ggsnStat.numMtStreamingFlowRejected ++;
            }
            else if (pdpInfo->qosInfo->trafficClass ==
                UMTS_QOS_CLASS_BACKGROUND)
            {
                gsnL3->ggsnStat.numMtBackgroundFlowRejected ++;
            }
            else
            {
                gsnL3->ggsnStat.numMtInteractiveFlowRejected ++;
            }

            // free queued packets
            while (!pdpInfo->inPacketBuf->empty())
            {
                Message* toFree = pdpInfo->inPacketBuf->front();
                pdpInfo->inPacketBuf->pop();
                gsnL3->agregPktBufSize
                    -= MESSAGE_ReturnPacketSize(toFree);
                MESSAGE_Free(node, toFree);

                // update statistics
                gsnL3->ggsnStat.numPsDataPacketDropped ++;
            }
            pdpInfo->pktBufSize = 0;
        }
        else
        {
            // update statistics
            gsnL3->ggsnStat.numMoFlowRejected ++;
            if (pdpInfo->qosInfo->trafficClass ==
                UMTS_QOS_CLASS_CONVERSATIONAL)
            {
                gsnL3->ggsnStat.numMoConversationalFlowRejected ++;
            }
            else if (pdpInfo->qosInfo->trafficClass ==
                UMTS_QOS_CLASS_STREAMING)
            {
                gsnL3->ggsnStat.numMoStreamingFlowRejected ++;
            }
            else if (pdpInfo->qosInfo->trafficClass ==
                UMTS_QOS_CLASS_BACKGROUND)
            {
                gsnL3->ggsnStat.numMoBackgroundFlowRejected ++;
            }
            else
            {
                gsnL3->ggsnStat.numMoInteractiveFlowRejected ++;
            }

        }
    }
    else // dropped
    {
        // dropped, marked it as to be deleted
        pdpInfo->status = UMTS_GGSN_PDP_REJECTED;
        pdpInfo->lastActiveTime = node->getNodeTime();

        if (pdpInfo->srcDestType == UMTS_FLOW_NETWORK_ORIGINATED)
        {
            // update statistics
            gsnL3->ggsnStat.numMtFlowDropped ++;
            if (pdpInfo->qosInfo->trafficClass ==
                UMTS_QOS_CLASS_CONVERSATIONAL)
            {
                gsnL3->ggsnStat.numMtConversationalFlowDropped ++;
            }
            else if (pdpInfo->qosInfo->trafficClass ==
                UMTS_QOS_CLASS_STREAMING)
            {
                gsnL3->ggsnStat.numMtStreamingFlowDropped ++;
            }
            else if (pdpInfo->qosInfo->trafficClass ==
                UMTS_QOS_CLASS_BACKGROUND)
            {
                gsnL3->ggsnStat.numMtBackgroundFlowDropped ++;
            }
            else
            {
                gsnL3->ggsnStat.numMtInteractiveFlowDropped ++;
            }

            // free queued packets
            while (!pdpInfo->inPacketBuf->empty())
            {
                Message* toFree = pdpInfo->inPacketBuf->front();
                pdpInfo->inPacketBuf->pop();
                gsnL3->agregPktBufSize
                    -= MESSAGE_ReturnPacketSize(toFree);
                MESSAGE_Free(node, toFree);

                // update statistics
                gsnL3->ggsnStat.numPsDataPacketDropped ++;
            }
            pdpInfo->pktBufSize = 0;
        }
        else
        {
             // update statistics
            gsnL3->ggsnStat.numMoFlowDropped ++;
            if (pdpInfo->qosInfo->trafficClass ==
                UMTS_QOS_CLASS_CONVERSATIONAL)
            {
                gsnL3->ggsnStat.numMoConversationalFlowDropped ++;
            }
            else if (pdpInfo->qosInfo->trafficClass ==
                UMTS_QOS_CLASS_STREAMING)
            {
                gsnL3->ggsnStat.numMoStreamingFlowDropped ++;
            }
            else if (pdpInfo->qosInfo->trafficClass ==
                UMTS_QOS_CLASS_BACKGROUND)
            {
                gsnL3->ggsnStat.numMoBackgroundFlowDropped ++;
            }
            else
            {
                gsnL3->ggsnStat.numMoInteractiveFlowDropped ++;
            }
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3GgsnHandleGtpMsg
// LAYER      :: Layer 3
// PURPOSE    :: Handle a GTP msg at GGSN
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + gsnL3     : UmtsLayer3Gsn*   : GSN layer 3 data
// + msg       : Message*         : Message containing the packet
// + interfaceIndex : int         : interface Index where the msg coming
// + srcAddr   : Address          : Source node address
// RETURN     :: void : NULL
// **/
static
void  UmtsLayer3GgsnHandleGtpMsg(Node *node,
                                UmtsLayer3Data *umtsL3,
                                UmtsLayer3Gsn* gsnL3,
                                Message* msg,
                                int inerfaceIndex,
                                Address srcAddr)
{
    UmtsGtpHeader gtpHeader;
    char extHeader[MAX_STRING_LENGTH];
    int extHeaderSize;
    UmtsRemoveGtpHeader(node, msg, &gtpHeader, extHeader, &extHeaderSize);
    switch (gtpHeader.msgType)
    {
        case UMTS_GTP_CREATE_PDP_CONTEXT_REQUEST:
        {
            UmtsLayer3GgsnGtpHandleCreatePDPContextRequest(
                node, umtsL3, gsnL3, msg, srcAddr,
                gtpHeader, extHeader, extHeaderSize);

            break;
        }
        case UMTS_GTP_DELETE_PDP_CONTEXT_REQUEST:
        {
            UmtsLayer3GsnGtpHandleDeletePDPContextRequest(
                node, umtsL3, gsnL3, msg, srcAddr,
                gtpHeader, extHeader, extHeaderSize);
            break;
        }
        case UMTS_GTP_DELETE_PDP_CONTEXT_RESPONSE:
        {
            UmtsLayer3GgsnGtpHandleDeletePDPContextResponse(
                node, umtsL3, gsnL3, msg, srcAddr,
                gtpHeader, extHeader, extHeaderSize);
            break;
        }
        case UMTS_GTP_UPDATE_PDP_CONTEXT_REQUEST:
        {
            break;
        }
        case UMTS_GTP_PDU_NOTIFICATION_RESPONSE:
        {
            UmtsLayer3GgsnGtpHandlePduNotificationResponse(
                node, umtsL3, gsnL3, msg, srcAddr,
                gtpHeader, extHeader, extHeaderSize);
            break;
        }
        case UMTS_GTP_G_PDU:
        {
            UmtsLayer3GgsnGtpHandleTunneledData(
                node, umtsL3, gsnL3, msg, gtpHeader, srcAddr);
            break;
        }
        default:
        {
            char errStr[MAX_STRING_LENGTH];
            sprintf(errStr,
                    "UMTS Layer3: Node%d(GGSN) receivers a "
                    "unknown GTP msg type %d",
                    node->nodeId,
                    gtpHeader.msgType);
            ERROR_ReportWarning(errStr);

            break;
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3SgsnHandleGtpMsg
// LAYER      :: Layer 3
// PURPOSE    :: Handle a GTP msg at SGSN
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + gsnL3     : UmtsLayer3Gsn*   : GSN layer 3 data
// + msg       : Message*         : Message containing the packet
// + interfaceIndex : int         : interface Index where the msg coming
// + srcAddr   : Address          : Source node address
// + info      : const char*      : additonal info
// RETURN     :: void : NULL
// **/
static
BOOL  UmtsLayer3SgsnHandleGtpMsg(Node *node,
                                UmtsLayer3Data *umtsL3,
                                UmtsLayer3Gsn* gsnL3,
                                Message* msg,
                                int inerfaceIndex,
                                Address srcAddr,
                                const char* info)
{
    BOOL msgFree = TRUE;
    UmtsGtpHeader gtpHeader;
    char extHeader[MAX_STRING_LENGTH];
    int extHeaderSize;

    NodeId ueId;
    memcpy(&ueId, &info[0], sizeof(NodeId));
    BOOL upLink = (BOOL)info[sizeof(NodeId)];
    UmtsRemoveGtpHeader(node, msg, &gtpHeader, extHeader, &extHeaderSize);
    switch (gtpHeader.msgType)
    {
        case UMTS_GTP_CREATE_PDP_CONTEXT_RESPONSE:
        {
            UmtsLayer3SgsnGtpHandleCreatePDPContextResponse(
                node, umtsL3, gsnL3, msg, srcAddr,
                gtpHeader, extHeader, extHeaderSize);

            break;
        }
        case UMTS_GTP_DELETE_PDP_CONTEXT_REQUEST:
        {
            // GGSN request to remove the flow
            UmtsLayer3GsnGtpHandleDeletePDPContextRequest(
                node, umtsL3, gsnL3, msg, srcAddr,
                gtpHeader, extHeader, extHeaderSize);
            break;
        }
        case UMTS_GTP_DELETE_PDP_CONTEXT_RESPONSE:
        {
            UmtsLayer3SgsnGtpHandleDeletePDPContextResponse(
                node, umtsL3, gsnL3, msg, srcAddr,
                gtpHeader, extHeader, extHeaderSize);

            break;
        }
        case UMTS_GTP_UPDATE_PDP_CONTEXT_RESPONSE:
        {
            break;
        }
        case UMTS_GTP_PDU_NOTIFICATION_REQUEST:
        {
            UmtsLayer3SgsnGtpHandlePduNotificationRequest(
                node, umtsL3, gsnL3, msg, srcAddr,
                gtpHeader, extHeader, extHeaderSize);
            break;
        }
        case UMTS_GTP_G_PDU:
        {
            msgFree = FALSE;
            UmtsLayer3SgsnGtpTunnelData(
                node, umtsL3, gsnL3, msg, gtpHeader.teId, srcAddr, upLink);

            break;
        }
        default:
        {
            char errStr[MAX_STRING_LENGTH];
            sprintf(errStr,
                    "UMTS Layer3: Node%d(GGSN) receivers a "
                    "unknown GTP msg type %d",
                    node->nodeId,
                    gtpHeader.msgType);
            ERROR_ReportWarning(errStr);

            break;
        }
    }

    return msgFree;
}

// /**
// FUNCTION   :: UmtsLayer3GsnHandleGtpMsg
// LAYER      :: Layer 3
// PURPOSE    :: Handle a GTP msg from GGSN/SGSN
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + gsnL3     : UmtsLayer3Gsn*   : GSN layer 3 data
// + msg       : Message*         : Message containing the packet
// + interfaceIndex : int         : interface Index where the msg coming
// + srcAddr   : Address          : Source node address
// + info      : const char*      : additonal info
// RETURN     :: void : NULL
// **/
static
BOOL  UmtsLayer3GsnHandleGtpMsg(Node *node,
                                UmtsLayer3Data *umtsL3,
                                UmtsLayer3Gsn* gsnL3,
                                Message* msg,
                                int interfaceIndex,
                                Address srcAddr,
                                const char* info)
{
    BOOL msgFree = TRUE;
    if (gsnL3->isGGSN)
    {
        UmtsLayer3GgsnHandleGtpMsg(node,
                                   umtsL3,
                                   gsnL3,
                                   msg,
                                   interfaceIndex,
                                   srcAddr);
        msgFree = FALSE;
    }
    else
    {
        msgFree = UmtsLayer3SgsnHandleGtpMsg(node,
                                   umtsL3,
                                   gsnL3,
                                   msg,
                                   interfaceIndex,
                                   srcAddr,
                                   info);
    }

    return msgFree;
}

// /**
// FUNCTION   :: UmtsLayer3GgsnCheckPdpPendingOnRouting
// LAYER      :: Layer 3
// PURPOSE    :: This function is called when getting routing info from HLR
// PARAMETERS ::
// + node      : Node*               : Pointer to node.
// + umtsL3    : UmtsLayer3Data *    : Pointer to UMTS Layer3 data
// + ueImsi    : CellularIMSI        : IMSI of the UE
// + locInfo   : UmtsHlrLocationInfo*: Location info of the UE from HLR.
//                                     NULL means HLR lookup failed
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3GgsnCheckPdpPendingOnRouting(Node *node,
                                            UmtsLayer3Data *umtsL3,
                                            CellularIMSI ueImsi,
                                            UmtsHlrLocationInfo *locInfo)
{
    UmtsLayer3Gsn *gsnL3 = (UmtsLayer3Gsn *) (umtsL3->dataPtr);
    std::list<UmtsLayer3GgsnPdpInfo*>::iterator it;
    UmtsLayer3GgsnPdpInfo *pdpInfo;

    // go thr PDP context list to see if anyone is waiting for routing info
    for (it = gsnL3->pdpInfoList->begin();
         it != gsnL3->pdpInfoList->end();
         it ++)
    {
        if ((*it)->status == UMTS_GGSN_PDP_ROUTING_PENDING &&
            (*it)->imsi == ueImsi)
        {
            // already in list
            pdpInfo = *it;

            if (locInfo != NULL)
            {
                // get routing info successfully
                // send PDU notification request to SGSN
                pdpInfo->sgsnId = locInfo->sgsnId;
                pdpInfo->sgsnAddr = locInfo->sgsnAddr;
                UmtsLayer3GsnGtpSendPduNotificationRequest(
                    node,
                    umtsL3,
                    gsnL3,
                    pdpInfo);

                // update PDP status
                pdpInfo->status = UMTS_GGSN_PDP_PDU_NOTIFY_PENDING;
            }
            else
            {
                // query routing info failed, mark flow as rejected
                pdpInfo->status = UMTS_GGSN_PDP_REJECTED;
                //Free stored packets
                while (!pdpInfo->inPacketBuf->empty())
                {
                    Message* toFree = pdpInfo->inPacketBuf->front();
                    pdpInfo->inPacketBuf->pop();
                    gsnL3->agregPktBufSize
                        -= MESSAGE_ReturnPacketSize(toFree);
                    MESSAGE_Free(node, toFree);
                    gsnL3->ggsnStat.numPsDataPacketDropped ++;
                }
                pdpInfo->pktBufSize = 0;
            } // if (locInfo)
        } //  if ((*it)->status)
    } // for
}

// /**
// FUNCTION   :: UmtsLayer3GgsnHandleCsPacketFromUpperOrOutside
// LAYER      :: Layer 3
// PURPOSE    :: Handle a packet of a CS flow from outside net
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + flowClsInfo: UmtsLayer3FlowClassifier* : Classifier of flow
// + msg       : Message*         : Message containing the packet
// + internal  : BOOL             : If the packet from internal UE
// RETURN     :: BOOL : msg need ot be reused or not
// **/
static
void UmtsLayer3GgsnHandleCsPacketFromUpperOrOutside(
         Node *node,
         UmtsLayer3Data *umtsL3,
         UmtsLayer3FlowClassifier* flowClsInfo,
         Message *msg,
         BOOL internal)
{
    // not implemented yet
    MESSAGE_Free(node, msg);
}

// /**
// FUNCTION   :: UmtsLayer3GgsnHandlePsPacketFromUpperOrOutside
// LAYER      :: Layer 3
// PURPOSE    :: Handle a timer expired msg
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + pdpInfo   : UmtsLayer3GgsnPdpInfo : PDP info stored at GGSN
// + msg       : Message*         : Message containing the packet
// + internal  : BOOL             : If the packet from an internal UE
// RETURN     :: BOOL : msg need ot be reused or not
// **/
static
void UmtsLayer3GgsnHandlePsPacketFromUpperOrOutside(
         Node *node,
         UmtsLayer3Data *umtsL3,
         UmtsLayer3GgsnPdpInfo* pdpInfo,
         Message *msg,
         BOOL internal)
{
    UmtsLayer3Gsn *gsnL3 = (UmtsLayer3Gsn *) (umtsL3->dataPtr);

    // update statistics
    if (!internal)
    {
        gsnL3->ggsnStat.numPsDataPacketFromOutsidePlmn ++;

        // update dynamic stat
        if (node->guiOption)
        {
            UmtsMeasElement measElem;
            measElem.measVal = MESSAGE_ReturnPacketSize(msg) *8;
            measElem.timeStamp = node->getNodeTime();

            // put to the meas Lsit
            gsnL3->ggsnStat.ingressThruputMeas->AddMeasElement(
                     measElem, node->getNodeTime());
        }
    }

    switch (pdpInfo->status)
    {
        case UMTS_GGSN_PDP_INIT: // just created, need to first query HLR
        {
            CellularIMSI ueImsi;
            UInt32 ueId;

            ueId = MAPPING_GetNodeIdFromInterfaceAddress(
                       node,
                       pdpInfo->uePdpAddr);

            if (ueId == INVALID_MAPPING)
            {
                // unknow UE
                char addStr[MAX_STRING_LENGTH];
                char errStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(&(pdpInfo->uePdpAddr), addStr);
                sprintf(errStr,
                        "UMTS: Node%d(GGSN) unable to "
                        "find UE with address %s",
                        node->nodeId,
                        addStr);

                ERROR_ReportWarning(errStr);
            }

            // enqueue the packet
            UmtsLayer3GgsnEnquePsPktFromOutside(
                    node,
                    gsnL3,
                    pdpInfo,
                    msg);
            pdpInfo->lastActiveTime = node->getNodeTime();

            // query HLR for routing info
            pdpInfo->ueId = ueId;
            pdpInfo->imsi.setId(ueId);
            ueImsi.setId(ueId);
            UmtsLayer3GsnQueryHlr(node, umtsL3, ueImsi);

            // update statistics
            gsnL3->ggsnStat.numRoutingQuerySent ++;

            // change the status to waitting for routing info
            pdpInfo->status = UMTS_GGSN_PDP_ROUTING_PENDING;

            break;
        }
        // active, no need to queue, forward to SGSN
        case UMTS_GGSN_PDP_ACTIVE:
        {
            pdpInfo->lastActiveTime = node->getNodeTime();
            UmtsLayer3GgsnGtpTunnelData(
                node,
                umtsL3,
                gsnL3,
                pdpInfo,
                msg);
            break;
        }

        case UMTS_GGSN_PDP_ROUTING_PENDING:
        case UMTS_GGSN_PDP_PDU_NOTIFY_PENDING:
        case UMTS_GGSN_PDP_PDU_NOTIFIED:
        {
            // PDP context not activated yet, just cache the packet
            UmtsLayer3GgsnEnquePsPktFromOutside(
                    node,
                    gsnL3,
                    pdpInfo,
                    msg);
            pdpInfo->lastActiveTime = node->getNodeTime();
            break;
        }

        case UMTS_GGSN_PDP_REJECTED: // invalid, free the packet
        {
            MESSAGE_Free(node, msg);
            //pdpInfo->lastActiveTime = node->getNodeTime();

            // update statistics
            gsnL3->ggsnStat.numPsDataPacketDropped ++;
            break;
        }

        default:
        {
            // unknow stat, print warning message and free packet
            char errStr[MAX_STRING_LENGTH];
            sprintf(errStr, "UMTS: Node%d(GGSN) unknown PDP stat %d\n",
                    node->nodeId,
                    pdpInfo->status);
            ERROR_ReportWarning(errStr);
            MESSAGE_Free(node, msg);

            // update statistics
            gsnL3->ggsnStat.numPsDataPacketDropped ++;
            break;
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3GsnHandleHelloPacket
// LAYER      :: Layer 3
// PURPOSE    :: Handle a hello packet from neighbor RNCs.
//               This is used to find out the RNCs connected
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + msg       : Message*         : Message containing the packet
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + interfaceIndex : int         : Interface from which packet was received
// + srcAddr   : Address          : Address of the source node
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3GsnHandleHelloPacket(Node *node,
                                    Message *msg,
                                    UmtsLayer3Data *umtsL3,
                                    int interfaceIndex,
                                    Address srcAddr)
{
    UmtsLayer3Gsn *gsnL3 = (UmtsLayer3Gsn *) (umtsL3->dataPtr);
    UmtsLayer3HelloPacket *helloPkt;

    helloPkt = (UmtsLayer3HelloPacket *) MESSAGE_ReturnPacket(msg);

    if (helloPkt->nodeType == CELLULAR_SGSN && gsnL3->isGGSN)
    {
        char* additionInfo = MESSAGE_ReturnPacket(msg) +
                             sizeof(UmtsLayer3HelloPacket);
        int additionInfoSize = MESSAGE_ReturnPacketSize(msg) -
                                sizeof(UmtsLayer3HelloPacket);
        int index = 0;

        while (index < additionInfoSize)
        {
            NodeAddress subnetAddress;
            int numHostBits;
            UmtsLayer3AddressInfo *addrInfo;
            memcpy(&subnetAddress,
                   &additionInfo[index],
                   sizeof(subnetAddress));
            index += sizeof(subnetAddress);
            memcpy(&numHostBits,
                   &additionInfo[index],
                   sizeof(numHostBits));
            index += sizeof(numHostBits);

            // if not in list, insert it
            addrInfo = (UmtsLayer3AddressInfo *)
                       FindItem(node,
                                gsnL3->addrInfoList,
                                0,
                                (char*)&subnetAddress,
                                sizeof(subnetAddress));
            if (addrInfo == NULL)
            {
                // not exist, allocate memory and insert
                addrInfo = (UmtsLayer3AddressInfo *)
                           MEM_malloc(sizeof(UmtsLayer3AddressInfo));
                memset(addrInfo, 0, sizeof(UmtsLayer3AddressInfo));
                addrInfo->subnetAddress = subnetAddress;
                addrInfo->numHostBits = numHostBits;
                ListInsert(node,
                           gsnL3->addrInfoList,
                           node->getNodeTime(),
                           addrInfo);
            }
        }
    }
    else if (helloPkt->nodeType == CELLULAR_RNC && !gsnL3->isGGSN)
    {
        if (DEBUG_BACKBONE)
        {
            printf("Node%d(GSN) discovered neighbor RNC %d from inf %d\n",
                   node->nodeId, helloPkt->nodeId, interfaceIndex);
        }

        UmtsLayer3RncInfo *itemPtr = NULL;

        itemPtr = (UmtsLayer3RncInfo *)
                  FindItem(node,
                           gsnL3->rncList,
                           0,
                           (char *) &(helloPkt->nodeId),
                           sizeof(helloPkt->nodeId));
        if (itemPtr == NULL)
        {
            // not exist, allocate memory and insert
            itemPtr = (UmtsLayer3RncInfo *)
                      MEM_malloc(sizeof(UmtsLayer3RncInfo));
            memset(itemPtr, 0, sizeof(UmtsLayer3RncInfo));
            itemPtr->nodeId = helloPkt->nodeId;
            ListInsert(node, gsnL3->rncList, node->getNodeTime(), itemPtr);

            gsnL3->rncCacheMap->insert(
                        std::make_pair(
                                itemPtr->nodeId,
                                new UmtsRncCacheInfo(
                                    node->nodeId)));
            // send a hello packet back
            UmtsLayer3SendHelloPacket(node, umtsL3, interfaceIndex);
        }
        else
        {
            char* additionInfo = MESSAGE_ReturnPacket(msg)
                                    + sizeof(UmtsLayer3HelloPacket);
            int additionInfoSize = MESSAGE_ReturnPacketSize(msg)
                                        - sizeof(UmtsLayer3HelloPacket);
            int index = 0;
            int numItems;
            memcpy(&numItems, &additionInfo[index], sizeof(numItems));
            index += sizeof(numItems);
            while (index < additionInfoSize && numItems > 0)
            {
                UInt32 cellId;
                NodeId nodebId;
                memcpy(&cellId,
                       &additionInfo[index],
                       sizeof(cellId));
                index += sizeof(cellId);
                memcpy(&nodebId,
                       &additionInfo[index],
                       sizeof(nodebId));
                index += sizeof(nodebId);
                numItems --;

                if (DEBUG_BACKBONE)
                {
                    printf("Node%d(GSN) received CELL info from RNC %d "
                           " CELLID: %u, NODEBID: %u \n",
                           node->nodeId, itemPtr->nodeId, cellId, nodebId);
                }
                gsnL3->cellCacheMap->insert(
                            std::make_pair(
                                    cellId,
                                    new UmtsCellCacheInfo(
                                        nodebId,
                                        itemPtr->nodeId)));
                UmtsLayer3GsnUpdateHlrCellInfo(
                        node,
                        umtsL3,
                        cellId,
                        nodebId,
                        itemPtr->nodeId);
                UmtsLayer3SetTimer(
                        node,
                        umtsL3,
                        UMTS_LAYER3_TIMER_HLR_CELL_UPDATE,
                        UMTS_LAYER3_TIMER_HLR_CELL_UPDATE_VAL,
                        NULL,
                        &cellId,
                        sizeof(cellId));
            }

            // retrieve subnet address info
            if (index < additionInfoSize)
            {
                memcpy(&numItems, &additionInfo[index], sizeof(numItems));
                index += sizeof(numItems);
            }
            BOOL addrInfoChanged = FALSE;
            while (index < additionInfoSize && numItems > 0)
            {
                NodeAddress subnetAddress;
                int numHostBits;
                UmtsLayer3AddressInfo *addrInfo;
                memcpy(&subnetAddress,
                       &additionInfo[index],
                       sizeof(subnetAddress));
                index += sizeof(subnetAddress);
                memcpy(&numHostBits,
                       &additionInfo[index],
                       sizeof(numHostBits));
                index += sizeof(numHostBits);
                numItems --;

                // if not in list, insert it
                addrInfo = (UmtsLayer3AddressInfo *)
                           FindItem(node,
                                    gsnL3->addrInfoList,
                                    0,
                                    (char*)&subnetAddress,
                                    sizeof(subnetAddress));
                if (addrInfo == NULL)
                {
                    // not exist, allocate memory and insert
                    addrInfo = (UmtsLayer3AddressInfo *)
                               MEM_malloc(sizeof(UmtsLayer3AddressInfo));
                    memset(addrInfo, 0, sizeof(UmtsLayer3AddressInfo));
                    addrInfo->subnetAddress = subnetAddress;
                    addrInfo->numHostBits = numHostBits;
                    ListInsert(node,
                               gsnL3->addrInfoList,
                               node->getNodeTime(),
                               addrInfo);
                    addrInfoChanged = TRUE;
                }
            }

            // if address info changed, set a timer to send a hello to GGSN
            if (addrInfoChanged)
            {
                UmtsLayer3SetTimer(
                    node,
                    umtsL3,
                    UMTS_LAYER3_TIMER_HELLO,
                    UMTS_LAYER3_TIMER_HELLO_DELAY,
                    NULL,
                    NULL,
                    0);
            }
        }
        itemPtr->nodeAddr = srcAddr;
        itemPtr->interfaceIndex = interfaceIndex;
    }
    else
    {
        // I am not supposed to receive hello packet from other type of node
        // Do nothing
        if (DEBUG_BACKBONE)
        {
            printf("Node%d(GSN) discovered nbor %d from inf %d\n",
                   node->nodeId, helloPkt->nodeId, interfaceIndex);
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3GsnHandleTimerMsg
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
BOOL UmtsLayer3GsnHandleTimerMsg(Node *node,
                                UmtsLayer3Data *umtsL3,
                                UmtsLayer3Gsn *gsnL3,
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
        case UMTS_LAYER3_TIMER_T3350:
        {
            msgReuse = UmtsGMmHandleTimerT3350(node, umtsL3, gsnL3, msg);

            break;
        }
        case UMTS_LAYER3_TIMER_T3255:
        {
            UmtsLayer3GsnHandleT3255(node, umtsL3, gsnL3, msg);

            break;
        }
        case UMTS_LAYER3_TIMER_T3313:
        {
            msgReuse = UmtsLayer3GsnHandleT3313(
                            node,
                            umtsL3,
                            gsnL3,
                            msg);

            break;
        }
         case UMTS_LAYER3_TIMER_T3385:
        {
            msgReuse = UmtsLayer3GsnHandleT3385(
                            node,
                            umtsL3,
                            gsnL3,
                            msg);

            break;
        }
        case UMTS_LAYER3_TIMER_T3395:
        {
            msgReuse = UmtsLayer3GsnHandleT3395(
                            node,
                            umtsL3,
                            gsnL3,
                            msg);

            break;
        }
        //CC timer
        case UMTS_LAYER3_TIMER_CC_T301:
        {
            UmtsLayer3MscHandleT301(
                node,
                umtsL3,
                gsnL3,
                msg);
            break;
        }
        case UMTS_LAYER3_TIMER_CC_T303:
        {
            UmtsLayer3MscHandleT303(
                node,
                umtsL3,
                gsnL3,
                msg);
            break;
        }
        case UMTS_LAYER3_TIMER_CC_T310:
        {
            UmtsLayer3MscHandleT310(
                node,
                umtsL3,
                gsnL3,
                msg);
            break;
        }
        case UMTS_LAYER3_TIMER_CC_T313:
        {
            UmtsLayer3MscHandleT313(
                node,
                umtsL3,
                gsnL3,
                msg);
            break;
        }
        case UMTS_LAYER3_TIMER_CC_T305:
        {
            UmtsLayer3MscHandleT305(
                node,
                umtsL3,
                gsnL3,
                msg);
            break;
        }
        case UMTS_LAYER3_TIMER_CC_T308:
        {
            UmtsLayer3MscHandleT308(
                node,
                umtsL3,
                gsnL3,
                msg);
            break;
        }
        case UMTS_LAYER3_TIMER_HLR_CELL_UPDATE:
        {
            msgReuse = UmtsLayer3GsnHandleCellUpTimer(
                            node,
                            umtsL3,
                            gsnL3,
                            msg);
            break;
        }
        case UMTS_LAYER3_TIMER_FLOW_TIMEOUT:
        {
            // check if any flow has timed out.
            UmtsLayer3GsnHandleFlowTimeout(node, umtsL3);

            // reset the timer.
            MESSAGE_Send(node, msg, UMTS_DEFAULT_FLOW_TIMEOUT_INTERVAL);
            msgReuse = TRUE;
            break;
        }
        case UMTS_LAYER3_TIMER_HELLO:
        {
            UmtsLayer3SgsnSendHelloMsgToGgsn(node, umtsL3, gsnL3->ggsnAddr);
            break;
        }
        case UMTS_LAYER3_TIMER_UPDATE_GUI:
        {
            UmtsLayer3GsnHandleUpdateGuiTimer(node, umtsL3, gsnL3);

            // reset the timer.
            MESSAGE_Send(node, msg, UMTS_DYNAMIC_STAT_AVG_TIME_WINDOW);
            msgReuse = TRUE;

            break;
        }
        default:
        {
            char errStr[MAX_STRING_LENGTH];
            sprintf(errStr,
                    "UMTS Layer3: Node%d(GSN) receivers a timer"
                    "event with unknown type %d",
                    node->nodeId,
                    timerType);
            ERROR_ReportWarning(errStr);

            break;
        }
    }

    return msgReuse;
}

//--------------------------------------------------------------------------
//  Key API functions
//--------------------------------------------------------------------------
// /**
// FUNCTION   :: UmtsLayer3GsnHandlePacket
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
void UmtsLayer3GsnHandlePacket(Node *node,
                               Message *msg,
                               UmtsLayer3Data *umtsL3,
                               int interfaceIndex,
                               Address srcAddr)
{
    UmtsLayer3Gsn *gsnL3 = (UmtsLayer3Gsn *) (umtsL3->dataPtr);

    UmtsBackboneMessageType backboneMsgType;
    char backboneInfo[UMTS_BACKBONE_HDR_MAX_INFO_SIZE];
    int infoSize = UMTS_BACKBONE_HDR_MAX_INFO_SIZE;
    UmtsRemoveBackboneHeader(node,
                             msg,
                             &backboneMsgType,
                             backboneInfo,
                             infoSize);

    switch (backboneMsgType)
    {
        case UMTS_BACKBONE_MSG_TYPE_HELLO:
        {
            // this is the hello packet for discovering connectivity
            char pd;
            char tiSpd;
            char msgType;
            UmtsLayer3RemoveHeader(node, msg, &pd, &tiSpd, &msgType);
            UmtsLayer3GsnHandleHelloPacket(node,
                                           msg,
                                           umtsL3,
                                           interfaceIndex,
                                           srcAddr);
            MESSAGE_Free(node, msg);
            break;
        }
        case UMTS_BACKBONE_MSG_TYPE_CSSIG:
        {
            NodeId ueId;
            char ti;
            char msgType;
            int index = 0;
            memcpy(&ueId, &backboneInfo[index], sizeof(ueId));
            index += sizeof(ueId);
            ti = backboneInfo[index++];
            msgType = backboneInfo[index++];
            if (gsnL3->isGGSN)
            {
                UmtsLayer3GmscHandleCsSigMsg(
                    node,
                    umtsL3,
                    ueId,
                    ti,
                    (UmtsCsSigMsgType)msgType,
                    MESSAGE_ReturnPacket(msg),
                    MESSAGE_ReturnPacketSize(msg),
                    srcAddr);
            }
            else
            {
                UmtsLayer3MscHandleCsSigMsg(
                    node,
                    umtsL3,
                    ueId,
                    ti,
                    (UmtsCsSigMsgType)msgType,
                    MESSAGE_ReturnPacket(msg),
                    MESSAGE_ReturnPacketSize(msg));
            }
            MESSAGE_Free(node, msg);
            break;
        }
        case UMTS_BACKBONE_MSG_TYPE_CSDATA:
        {
            NodeId ueId;
            char ti;
            int index = 0;
            memcpy(&ueId, &backboneInfo[index], sizeof(ueId));
            index += sizeof(ueId);
            ti = backboneInfo[index++];
            if (gsnL3->isGGSN)
            {
                UmtsLayer3GmscHandleCsPkt(
                    node,
                    umtsL3,
                    ueId,
                    ti,
                    msg);
            }
            else
            {
                UmtsLayer3MscHandleCsPkt(
                    node,
                    umtsL3,
                    ueId,
                    ti,
                    msg);
            }
            break;
        }
        case UMTS_BACKBONE_MSG_TYPE_IU_CSDATA:
        {
            NodeId ueId;
            char rabId;
            int index = 0;
            memcpy(&ueId, &backboneInfo[index], sizeof(ueId));
            index += sizeof(ueId);
            rabId = backboneInfo[index++];
            UmtsLayer3MscHandleCsPktFromRnc(
                node,
                umtsL3,
                ueId,
                rabId,
                msg);
            break;
        }
        case UMTS_BACKBONE_MSG_TYPE_GTP:
        {
            //Receive a message containing GTP data/signalling
            BOOL msgFree = UmtsLayer3GsnHandleGtpMsg(
                node, umtsL3, gsnL3, msg,
                interfaceIndex,srcAddr, backboneInfo);
            if (msgFree)
            {
                MESSAGE_Free(node, msg);
            }
            break;
        }
        case UMTS_BACKBONE_MSG_TYPE_IU:
        {
            //Receive a message from RNC containing RANAP signalling PDU
            UmtsRanapMessageType ranapMsgType = (UmtsRanapMessageType)
                                                backboneInfo[0];
            NodeId nodeId;
            memcpy(&nodeId, &backboneInfo[1], sizeof(NodeId));
            UmtsLayer3GsnHandleRanapMsg(node,
                                        umtsL3,
                                        msg,
                                        ranapMsgType,
                                        nodeId,
                                        interfaceIndex,
                                        srcAddr);
            break;
        }
        case UMTS_BACKBONE_MSG_TYPE_GR:
        case UMTS_BACKBONE_MSG_TYPE_GC:
        {
            UmtsHlrCmdTgt target = (UmtsHlrCmdTgt)backboneInfo[0];
            //Receive a signalling message from HLR
            if (target == UMTS_HLR_COMMAND_MS)
            {
                UmtsLayer3GsnHandlePacketFromHlr(
                    node,
                    umtsL3,
                    MESSAGE_ReturnPacket(msg));
            }
            else if (target == UMTS_HLR_COMMAND_CELL)
            {
                UmtsLayer3GsnHandleHlrCellReply(
                    node,
                    umtsL3,
                    MESSAGE_ReturnPacket(msg));
            }
            else
            {
                ERROR_ReportError("Received a message with invalid"
                    " command target.");
            }
            MESSAGE_Free(node,msg);
            break;
        }
        case UMTS_BACKBONE_MSG_TYPE_IUR_FORWARD:
        {
            NodeId destRncId;
            memcpy(&destRncId, backboneInfo, sizeof(destRncId));
            UmtsLayer3SgsnSendRanapMsgToRnc(
                    node,
                    umtsL3,
                    msg,
                    destRncId,
                    UMTS_RANAP_MSG_TYPE_IUR_MESSAGE_FORWARD,
                    destRncId);
            break;
        }
        default:
        {
            char infoStr[MAX_STRING_LENGTH];
            sprintf(infoStr,
                    "UMTS Layer3: Node%d(GSN) receives a backbone message "
                    "with an unknown Backbone message type as %d",
                    node->nodeId,
                    backboneMsgType);
            ERROR_ReportWarning(infoStr);
        }
    }
}

// /**
// FUNCTION   :: UmtsLayer3GgsnHandlePacketFromUpperOrOutside
// LAYER      :: Layer3
// PURPOSE    :: Handle packets from outside PDNs to the cellular
// PARAMETERS ::
// + node      : Node*    : Pointer to node.
// + msg       : Message* : Message to be handled
// + inIfIndex : int      : Interface from which the packet was received
// + netType   : int      : network Type
// + internal  : BOOL     : If the packet from internal UE
// RETURN     :: void     : NULL
// **/
void UmtsLayer3GgsnHandlePacketFromUpperOrOutside(
         Node* node,
         Message* msg,
         int inIfIndex,
         int netType,
         BOOL internal)
{
    UmtsLayer3Data *umtsL3 = UmtsLayer3GetData(node);
    UmtsLayer3Gsn *gsnL3 = (UmtsLayer3Gsn *) (umtsL3->dataPtr);
    std::list<UmtsLayer3GgsnPdpInfo*>::iterator it;
    UmtsLayer3GgsnPdpInfo* pdpInfo = NULL;

    UmtsLayer3FlowClassifier flowClsInfo;
    const char* payload;
    TosType tos = 0;
    TraceProtocolType appType;

    if (DEBUG_PACKET)
    {
        char addrStr[256];
        printf("UMTS:Node%d(GGSN) receives a packet from upper or outside "
               "from interface %d networkType %d\n",
               node->nodeId,
               inIfIndex,
               netType);
        if (netType == NETWORK_IPV4)
        {
            IpHeaderType *ipHeader = (IpHeaderType *)
                                     MESSAGE_ReturnPacket(msg);
            printf("    ip_p = %d\n", ipHeader->ip_p);
            IO_ConvertIpAddressToString(ipHeader->ip_dst, addrStr);
            printf("    ip_dst = %s\n", addrStr);
        }
    }

    //Set the message as cellular message, so that it cannot be
    //handled as IP packet during transmission
    MESSAGE_SetLayer(msg, NETWORK_LAYER, NETWORK_PROTOCOL_CELLULAR);

    // first, get info for classification from the packet
    payload = MESSAGE_ReturnPacket(msg);
    memset(&flowClsInfo, 0, sizeof(flowClsInfo));
    UmtsLayer3BuildFlowClassifierInfo(
        node,
        inIfIndex,
        netType,
        msg,
        &tos,
        &flowClsInfo,
        &payload);

    // if packet of CS flow, handle it
    if (flowClsInfo.domainInfo == UMTS_LAYER3_CN_DOMAIN_CS)
    {
        // packet from other PLMN or PSTN
        UmtsLayer3GgsnHandleCsPacketFromUpperOrOutside(
            node,
            umtsL3,
            &flowClsInfo,
            msg,
            internal);

        return;
    }

    // check packet size or other QoS paramenters
    if (!UmtsLayer3CheckFlowQoSParam(
            node, inIfIndex,
            (TraceProtocolType) msg->originatingProtocol,
            tos, &payload))
    {
        MESSAGE_Free(node, msg);

        gsnL3->ggsnStat.numPsDataPacketDropped ++;
        gsnL3->ggsnStat.numPsDataPacketDroppedUnsupportedFormat ++;

        return;
    }

    // PS packet from other PLMN or PDN,
    // check if the flow already classified before
    for (it = gsnL3->pdpInfoList->begin();
         it != gsnL3->pdpInfoList->end();
         it ++)
    {
        if ((*it)->classifierInfo != NULL &&
            memcmp((*it)->classifierInfo, &flowClsInfo, sizeof(flowClsInfo))
            == 0)
        {
            // already in list
            pdpInfo = *it;
            break;
        }
    }

    if (pdpInfo == NULL)
    {
        // this is a new flow, need to establish SM connection first
        pdpInfo = UmtsLayer3GgsnAddPdpInfoToList(
                      node, umtsL3, gsnL3, NULL, NULL,
                      (UInt32*)&(gsnL3->teIdCount), NULL);

        // update Qos
        memcpy(pdpInfo->classifierInfo,
               &flowClsInfo,
               sizeof(UmtsLayer3FlowClassifier));

        pdpInfo->uePdpAddr = flowClsInfo.dstAddr;
        pdpInfo->srcDestType = UMTS_FLOW_NETWORK_ORIGINATED;

        // increase teIdCount
        gsnL3->teIdCount ++;

        // // get/update QoS  info
        appType = (TraceProtocolType) msg->originatingProtocol;

        UmtsLayer3GetFlowQoSInfo(
            node, inIfIndex, appType,
            tos, &payload, &(pdpInfo->qosInfo));

        if (DEBUG_QOS)
        {
            char clkStr[MAX_STRING_LENGTH];

            if (pdpInfo->classifierInfo->domainInfo ==
                UMTS_LAYER3_CN_DOMAIN_CS)
            {
                UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_CC);
            }
            else if (pdpInfo->classifierInfo->domainInfo ==
                     UMTS_LAYER3_CN_DOMAIN_PS)
            {
                UmtsLayer3PrintRunTimeInfo(node, UMTS_LAYER3_SUBLAYER_SM);

            }
            TIME_PrintClockInSecond(
                pdpInfo->qosInfo->transferDelay, clkStr);

            printf("\n\t handle packet from upper for a new flow\n");
            printf("\n\t -------QoS Info------");
            printf("\n\t trafficClass %d \t UL maxBitRate %d"
                 "\t UL guaranteedBitRate %d",
                 pdpInfo->qosInfo->trafficClass,
                 pdpInfo->qosInfo->ulMaxBitRate,
                 pdpInfo->qosInfo->ulGuaranteedBitRate);
            printf("\n\t DL maxBitRate %d DL guaranteedBitRate %d \n"
                 "\t sduErrorRatio %f \t residualBER %f",
                 pdpInfo->qosInfo->dlMaxBitRate,
                 pdpInfo->qosInfo->dlGuaranteedBitRate,
                 (double)pdpInfo->qosInfo->sduErrorRatio,
                 pdpInfo->qosInfo->residualBER);
           printf(" \t transferDelay %s\n",
                 clkStr);
        }

        // update statistics
        gsnL3->ggsnStat.numMtFlowRequested ++;
        if (pdpInfo->qosInfo->trafficClass ==
            UMTS_QOS_CLASS_CONVERSATIONAL)
        {
            gsnL3->ggsnStat.numMtConversationalFlowRequested ++;
        }
        else if (pdpInfo->qosInfo->trafficClass ==
            UMTS_QOS_CLASS_STREAMING)
        {
            gsnL3->ggsnStat.numMtStreamingFlowRequested ++;
        }
        else if (pdpInfo->qosInfo->trafficClass ==
            UMTS_QOS_CLASS_BACKGROUND)
        {
            gsnL3->ggsnStat.numMtBackgroundFlowRequested ++;
        }
        else
        {
            gsnL3->ggsnStat.numMtInteractiveFlowRequested ++;
        }
    }

    // packet from other PLMN or PDN
    UmtsLayer3GgsnHandlePsPacketFromUpperOrOutside(
        node,
        umtsL3,
        pdpInfo,
        msg,
        internal);
}
// /**
// FUNCTION   :: UmtsLayer3GsnInitDynamicStats
// LAYER      :: Layer3
// PURPOSE    :: Initiate dynamic statistics
// PARAMETERS ::
// + node      : Node*         : Pointer to node
// + interfaceIdnex   : UInt32 : interface index of UMTS
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + gsnL3     : UmtsLayer3Gsn *  : POinter to PHY UMTS GSN data
// RETURN     :: void : NULL
// **/
static
void UmtsLayer3GsnInitDynamicStats(Node *node,
                                   UInt32 interfaceIndex,
                                   UmtsLayer3Data *umtsL3,
                                   UmtsLayer3Gsn *gsnL3)
{
    if (!node->guiOption)
    {
        return;
    }

    // start a update GUI timer
    UmtsLayer3SetTimer(node, umtsL3,
                       UMTS_LAYER3_TIMER_UPDATE_GUI,
                       UMTS_DYNAMIC_STAT_AVG_TIME_WINDOW,
                       NULL, NULL, 0);
    if (gsnL3->isGGSN)
    {
        clocktype twSize = UMTS_DYNAMIC_STAT_AVG_TIME_WINDOW;

        // init the timw windows measurement
        gsnL3->ggsnStat.egressThruputMeas = new UmtsMeasTimeWindow(twSize);
        gsnL3->ggsnStat.ingressThruputMeas = new UmtsMeasTimeWindow(twSize);

        // GGSN
#if 0
         gsnL3->ggsnStat.numIngressConnGuiId =
            GUI_DefineMetric("GGSN: Number of Ingress Connections",
                         node->nodeId,
                         GUI_NETWORK_LAYER,
                         interfaceIndex,
                         GUI_INTEGER_TYPE,
                         GUI_CUMULATIVE_METRIC);

         gsnL3->ggsnStat.numEgressConnGuiId =
            GUI_DefineMetric("GGSN: Number of Egress Connections",
                         node->nodeId,
                         GUI_NETWORK_LAYER,
                         interfaceIndex,
                         GUI_INTEGER_TYPE,
                         GUI_CUMULATIVE_METRIC);
#endif

         gsnL3->ggsnStat.ingressThruputGuiID =
            GUI_DefineMetric("UMTS GGSN: Ingress Throughput (bps)",
                         node->nodeId,
                         GUI_NETWORK_LAYER,
                         interfaceIndex,
                         GUI_DOUBLE_TYPE,
                         GUI_CUMULATIVE_METRIC);

         gsnL3->ggsnStat.egressThruputGuiId =
            GUI_DefineMetric("UMTS GGSN: Egress Throughput (bps)",
                         node->nodeId,
                         GUI_NETWORK_LAYER,
                         interfaceIndex,
                         GUI_DOUBLE_TYPE,
                         GUI_CUMULATIVE_METRIC);
    }
    else
    {
        // SGSN
        gsnL3->sgsnStat.numRegedUeGuiId =
            GUI_DefineMetric("UMTS SGSN: Number of Registered UEs",
                         node->nodeId,
                         GUI_NETWORK_LAYER,
                         interfaceIndex,
                         GUI_INTEGER_TYPE,
                         GUI_CUMULATIVE_METRIC);
    }
}

// /**
// FUNCTION   :: UmtsLayer3GsnInit
// LAYER      :: Layer 3
// PURPOSE    :: Initialize GSN data at UMTS layer 3 data.
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + nodeInput : const NodeInput* : Pointer to node input.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// RETURN     :: void : NULL
// **/
void UmtsLayer3GsnInit(Node *node,
                       const NodeInput *nodeInput,
                       UmtsLayer3Data *umtsL3)
{
    UmtsLayer3Gsn *gsnL3;

    // initialize the basic UMTS layer3 data
    gsnL3 = (UmtsLayer3Gsn *) MEM_malloc(sizeof(UmtsLayer3Gsn));
    ERROR_Assert(gsnL3 != NULL, "UMTS: Out of memory!");
    memset(gsnL3, 0, sizeof(UmtsLayer3Gsn));

    umtsL3->dataPtr = (void *) gsnL3;

    if (umtsL3->nodeType == CELLULAR_GGSN)
    {
        gsnL3->isGGSN = TRUE;
    }

    // set some properties
    gsnL3->gsnId = node->nodeId;

    // read configuration parameters
    UmtsLayer3GsnInitParameter(node, nodeInput, gsnL3);

    if (DEBUG_PARAMETER)
    {
        UmtsLayer3GsnPrintParameter(node, gsnL3);
    }

    // init dyanmic stat
    UmtsLayer3GsnInitDynamicStats(
        node, umtsL3->interfaceIndex, umtsL3, gsnL3);

    // initialize the list for RNCs connected to me
    ListInit(node, &(gsnL3->rncList));
    ListInit(node, &(gsnL3->addrInfoList));

    // initialize the VLR map
    gsnL3->vlrMap = new UmtsVlrMap;

    // SGSN init Ue list
    if (!gsnL3->isGGSN)
    {
        gsnL3->ueInfoList = new std::list<UmtsSgsnUeDataInSgsn*>;
    }
    else
    {
        gsnL3->pdpInfoList = new std::list<UmtsLayer3GgsnPdpInfo*>;
        gsnL3->callFlowList = new UmtsGmscCallFlowList;
        gsnL3->ueLocCacheMap = new UmtsUeLocEntryMap;
    }
    gsnL3->agregPktBufSize = 0;

    gsnL3->cellCacheMap = new UmtsCellCacheMap;
    gsnL3->rncCacheMap = new UmtsRncCacheMap;
    // set a Timer to check the out of status PS app
    if (gsnL3->isGGSN)
    {
        gsnL3->flowTimeoutTimer = UmtsLayer3SetTimer(
                                      node,
                                      umtsL3,
                                      UMTS_LAYER3_TIMER_FLOW_TIMEOUT,
                                      UMTS_DEFAULT_FLOW_TIMEOUT_INTERVAL,
                                      NULL);
    }
}

// /**
// FUNCTION   :: UmtsLayer3GsnLayer
// LAYER      :: Layer 3
// PURPOSE    :: Handle GSN specific timers and layer messages.
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + msg       : Message*         : Message for node to interpret
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// RETURN     :: void : NULL
// **/
void UmtsLayer3GsnLayer(Node *node, Message *msg, UmtsLayer3Data *umtsL3)
{
    UmtsLayer3Gsn *gsnL3 = (UmtsLayer3Gsn *) (umtsL3->dataPtr);

    BOOL msgReused = FALSE;

    switch ((msg->eventType))
    {
        case MSG_NETWORK_CELLULAR_TimerExpired:
        {
            msgReused =
                UmtsLayer3GsnHandleTimerMsg(node, umtsL3, gsnL3, msg);

            break;
        }

        default:
        {
            char errStr[MAX_STRING_LENGTH];
            sprintf(errStr,
                    "UMTS Layer3: Node%d(GSN) got an event with a "
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
// FUNCTION   :: UmtsLayer3GsnFinalize
// LAYER      :: Layer 3
// PURPOSE    :: Print GSN specific stats and clear protocol variables.
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// RETURN     :: void : NULL
// **/
void UmtsLayer3GsnFinalize(Node *node, UmtsLayer3Data *umtsL3)
{
    UmtsLayer3Gsn *gsnL3 = (UmtsLayer3Gsn *) (umtsL3->dataPtr);

    for (UmtsCellCacheMap::iterator it = gsnL3->cellCacheMap->begin();
         it != gsnL3->cellCacheMap->end(); ++it)
    {
        delete it->second;
    }
    delete gsnL3->cellCacheMap;

    for (UmtsRncCacheMap::iterator it = gsnL3->rncCacheMap->begin();
         it != gsnL3->rncCacheMap->end(); ++it)
    {
        delete it->second;
    }
    delete gsnL3->rncCacheMap;

    // print GSN specific statistics
    if (gsnL3->isGGSN)
    {
        if (umtsL3->collectStatistics)
        {
            UmtsLayer3GgsnPrintStats(node, umtsL3, gsnL3);
        }

        UmtsClearStlContDeep(gsnL3->callFlowList);
        delete gsnL3->callFlowList;

        for (UmtsUeLocEntryMap::iterator it = gsnL3->ueLocCacheMap->begin();
             it != gsnL3->ueLocCacheMap->end(); ++it)
        {
            delete it->second;
        }
        delete gsnL3->ueLocCacheMap;

        if (node->guiOption)
        {
            delete gsnL3->ggsnStat.egressThruputMeas;
            delete gsnL3->ggsnStat.ingressThruputMeas;
        }

        // free memory
        std::list<UmtsLayer3GgsnPdpInfo*>::iterator itPdp;

        for (itPdp = gsnL3->pdpInfoList->begin();
             itPdp != gsnL3->pdpInfoList->end();
             itPdp ++)
        {
            UmtsLayer3GgsnPdpInfoFinalize(node, gsnL3, (*itPdp));
            *itPdp = NULL;
        }
        gsnL3->pdpInfoList->remove(NULL);
        delete gsnL3->pdpInfoList;
    }
    else
    {
        if (umtsL3->collectStatistics)
        {
            UmtsLayer3SgsnPrintStats(node, umtsL3, gsnL3);
        }

        // free memory
        std::list<UmtsSgsnUeDataInSgsn*>::iterator itUe;
        std::list<UmtsSgsnUeDataInSgsn*>::iterator itUeTemp;
        for (itUe = gsnL3->ueInfoList->begin();
             itUe != gsnL3->ueInfoList->end();)
        {
            itUeTemp = itUe ++;
            UmtsLayer3GsnRemoveUeInfo(
                node, umtsL3, gsnL3, (*itUeTemp)->ueId);
        }
        delete gsnL3->ueInfoList;
    }

    //delete VLR
    UmtsVlrMap::iterator pos;
    UmtsVlrMap::iterator posTemp;
    for (pos = gsnL3->vlrMap->begin(); pos != gsnL3->vlrMap->end();)
    {
        UmtsVlrEntry * vlrEntry;
        posTemp = pos ++ ;
        vlrEntry = (UmtsVlrEntry *) posTemp->second;
        MEM_free(vlrEntry);
        gsnL3->vlrMap->erase(posTemp);
    }
    delete gsnL3->vlrMap;

    // Finalize the list for RNCs connected to me
    ListFree(node, gsnL3->rncList, FALSE);
    ListFree(node, gsnL3->addrInfoList, FALSE);


    MEM_free(gsnL3);
    umtsL3->dataPtr = NULL;
}

// FUNCTION   :: UmtsLayer3GsnMmConnEstInd
// LAYER      :: Layer3 MM
// PURPOSE    :: Request to establish MM connection by CM entities
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data*   : Pointer to L3 data
// + ueNas            : UmtsSgsnUeDataInSgsn* : Pointer to NAS
// RETURN     :: void
// **/
static
void UmtsLayer3GsnMmConnEstInd(
        Node* node,
        UmtsLayer3Data *umtsL3,
        UmtsSgsnUeDataInSgsn* ueNas)
{
    //for each CM entity, send messages to be send
    UmtsMmNwData* ueMm = ueNas->mmData;
    if (ueMm->mainState == UMTS_MM_NW_WAIT_FOR_RR_CONNECTION)
    {
        std::list<UmtsLayer3SgsnFlowInfo*>::iterator ueFlowIter;
        for (ueFlowIter = ueNas->appInfoList->begin();
             ueFlowIter != ueNas->appInfoList->end(); ++ueFlowIter)
        {
            UmtsLayer3SgsnFlowInfo* ueFlow = *ueFlowIter;
            if (ueFlow->classifierInfo->domainInfo ==
                UMTS_LAYER3_CN_DOMAIN_CS)
            {
                UmtsCcNwData* ueCc = ueFlow->cmData.ccData;
                if (ueCc->state == UMTS_CC_NW_MM_CONNECTION_PENDING)
                {
                    UmtsLayer3MscSendMobTermCallSetupToUe(
                        node,
                        umtsL3,
                        ueNas,
                        ueFlow);
                    ueCc->state = UMTS_CC_NW_CALL_PRESENT;

                    UmtsLayer3MscSendVoiceCallRabAssgt(
                            node,
                            umtsL3,
                            ueNas->ueId,
                            ueFlow->transId);
                }

                //Add current CC-TI into active MM connections
                UmtsLayer3MmConn cmpMmConn = UmtsLayer3MmConn(
                                                UMTS_LAYER3_PD_CC,
                                                ueFlow->transId);
                if (ueMm->actMmConns->end() ==
                        find_if(ueMm->actMmConns->begin(),
                                ueMm->actMmConns->end(),
                                bind2nd(
                                  UmtsPointedItemEqual<UmtsLayer3MmConn>(),
                                        &cmpMmConn)))
                {
                    ueMm->actMmConns->
                        push_back(new UmtsLayer3MmConn(cmpMmConn));
                }
            } // if (ueFlow->classifierInfo->domainInfo)
        } // for (ueFlowIter)
    } // if (ueMm->mainState)
}

// FUNCTION   :: UmtsLayer3GsnMmConnRelInd
// LAYER      :: Layer3 MM
// PURPOSE    :: Request to establish MM connection by CM entities
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + umtsL3           : UmtsLayer3Data*   : Pointer to L3 data
// + ueNas            : UmtsSgsnUeDataInSgsn* : Pointer to NAS
// + pd               : UmtsLayer3PD      : Protocol discrimator
// + ti               : unsigned char     : Transaction ID
// RETURN     :: BOOL : TRUE if the MM connection is active
// **/
static
void UmtsLayer3GsnMmConnRelInd(
        Node* node,
        UmtsLayer3Data *umtsL3,
        UmtsSgsnUeDataInSgsn* ueNas,
        UmtsLayer3PD pd,
        unsigned char ti)
{
    UmtsLayer3Gsn* gsnL3 = (UmtsLayer3Gsn*)(umtsL3->dataPtr);
    UmtsLayer3SgsnFlowInfo* flowInfo =
        UmtsLayer3GsnFindAppInfoFromList(
                node,
                gsnL3,
                ueNas,
                ti);
    if (flowInfo)
    {
        if (pd == UMTS_LAYER3_PD_CC)
        {
            UmtsCcNwData* ueCc = flowInfo->cmData.ccData;
            if (ueCc->state != UMTS_CC_NW_NULL
                && ueCc->state != UMTS_CC_NW_DISCONNECT_INDICATION
                && ueCc->state != UMTS_CC_NW_RELEASE_REQUEST)
            {
                UmtsLayer3MscSendDisconnectToGmsc(
                    node,
                    umtsL3,
                    ueNas->ueId,
                    flowInfo->transId,
                    UMTS_CALL_CLEAR_DROP,
                    ueCc->peerAddr);
            }
        }
        UmtsLayer3GsnRemoveAppInfoFromList(
            node,
            gsnL3,
            ueNas->transIdBitSet,
            ueNas->appInfoList,
            flowInfo->srcDestType,
            flowInfo->transId);
    }
}

// /**
// FUNCTION   :: UmtsLayer3GgsnIsPacketForMyPlmn
// LAYER      :: Layer3
// PURPOSE    :: Check if destination of the packet is in my PLMN
// PARAMETERS ::
// + node      : Node*    : Pointer to node.
// + msg       : Message* : Packet to be checked
// + inIfIndex : int      : Interface from which the packet is received
// + netType   : int      : network type. IPv4 or IPv6
// RETURN     :: BOOL     : TRUE if pkt for my PLMN, FALSE otherwise
// **/
BOOL UmtsLayer3GgsnIsPacketForMyPlmn(Node* node,
                                     Message* msg,
                                     int inIfIndex,
                                     int netType)
{
    UmtsLayer3Data *umtsL3 = UmtsLayer3GetData(node);
    UmtsLayer3Gsn *gsnL3 = (UmtsLayer3Gsn *) (umtsL3->dataPtr);

    if (!gsnL3->isGGSN)
    {
        return FALSE;
    }

    if (inIfIndex < 0 || inIfIndex >= node->numberInterfaces)
    {
        return FALSE;
    }

    if (netType == NETWORK_IPV4)
    {
        // get IPv4 header
        IpHeaderType ipHeader;
        int outgoingInterfaceToUse;
        NodeAddress outgoingBroadcastAddress;

        memcpy(&ipHeader,  MESSAGE_ReturnPacket(msg), sizeof(ipHeader));


        if ((ipHeader.ip_p != IPPROTO_UDP &&
            ipHeader.ip_p != IPPROTO_TCP) ||
            ipHeader.ip_dst == ANY_DEST ||
            IsOutgoingBroadcast(node,
                                ipHeader.ip_dst,
                                &outgoingInterfaceToUse,
                                &outgoingBroadcastAddress))
        {
            // broadcast packet, won't forward
            return FALSE;
        }
    }
    else if (netType == NETWORK_IPV6)
    {
    }
    else
    {
        // don't support other network
        return FALSE;
    }

    // in a completed implementation, a PLMN may have multiple GGSNs and
    // each GGSN may in charge of different address domain. And GGSN knows
    // addresses allocated to UEs. So in this function, the GGSN should
    // only return TRUE only when an UE in this PLMN has the address same
    // as the destination of the IP packet or fall in the multicast group.

    // in this implementation, GGSN maintains a list address info reported
    // from NodeB-->RNC-->SGSN-->GGSN
    ListItem *item = gsnL3->addrInfoList->first;
    while (item)
    {
        UmtsLayer3AddressInfo *addrInfo;

        addrInfo = (UmtsLayer3AddressInfo *)item->data;
        if (netType == NETWORK_IPV4)
        {
            NodeAddress myPdpNetworkAddr;
            NodeAddress destNetworkAddr;
            IpHeaderType ipHeader;

            myPdpNetworkAddr =
                MaskIpAddress(
                    addrInfo->subnetAddress,
                    ConvertNumHostBitsToSubnetMask(addrInfo->numHostBits));

            memcpy(&ipHeader, MESSAGE_ReturnPacket(msg), sizeof(ipHeader));
            destNetworkAddr =
                MaskIpAddress(
                    ipHeader.ip_dst,
                    ConvertNumHostBitsToSubnetMask(addrInfo->numHostBits));

            if (myPdpNetworkAddr == destNetworkAddr)
            {
                return TRUE;
            }
        }
        else if (netType == NETWORK_IPV6)
        {
            // ToDo for IPv6
        }
        item = item->next;
    }

    return FALSE;
}
