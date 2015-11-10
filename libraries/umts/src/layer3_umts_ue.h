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

#ifndef _LAYER3_UMTS_UE_H_
#define _LAYER3_UMTS_UE_H_

#include <bitset>
#include <list>
#include <queue>

#include "umts_constants.h"
#include "layer3_umts.h"

// /**
// CONSTANT    :: UMTS_LAYER3_UE_START_JITTER : 1MS 
// DESCRIPTION :: Used for generating a random delay before power on an UE
// **/
#define UMTS_LAYER3_UE_START_JITTER    (1 * MICRO_SECOND)


// /**
// ENUM        :: UmtsMmMsState
// DESCRIPTION :: MM sublayer main states in the mobile station
// **/
typedef enum
{
    UMTS_MM_MS_NULL = 0,
    UMTS_MM_MS_LOCATION_UPDATING_INITIATED = 3,
    UMTS_MM_MS_WAIT_FOR_OUTGOING_MM_CONNECTION = 5,
    UMTS_MM_MS_MM_CONNECTION_ACTIVE = 6,
    UMTS_MM_MS_IMSI_DETACH_INITIATED = 7,
    UMTS_MM_MS_PROCESS_CM_SERVICE_PROMPT = 8,
    UMTS_MM_MS_WAIT_FOR_NETWORK_COMMAND = 9,
    UMTS_MM_MS_LOCATION_UPDATE_REJECTED = 10,
    UMTS_MM_MS_WAIT_FOR_RR_CONNECTION_LOCATION_UPDATING = 13,
    UMTS_MM_MS_WAIT_FOR_RR_CONNECTION_MM_CONNECTION = 14,
    UMTS_MM_MS_WAIT_FOR_RR_CONNECTION_IMSI_DETACH = 15,
    UMTS_MM_MS_WAIT_FOR_REESTABLISH = 17,
    UMTS_MM_MS_WAIT_FOR_RR_ACTIVE = 18,
    UMTS_MM_MS_MM_IDLE = 19,
    UMTS_MM_MS_WAIT_FOR_ADDITIONAL_OUTGOING_MM_CONNECTION = 20,
    UMTS_MM_MS_MM_CONNECTION_ACTIVE_GROUP_TRANSMIT_MODE = 21,
    UMTS_MM_MS_WAIT_FOR_RR_CONNECTION_GROUP_TRANSMIT_MODE = 22,
    UMTS_MM_MS_LOCATION_UPDATING_PENDING = 23,
    UMTS_MM_MS_IMSI_DETACH_PENDING = 24,
    UMTS_MM_MS_RR_CONNECTION_RELEASE_NOT_ALLOWED = 25
} UmtsMmMsState;

// /**
// ENUM        :: UmtsMmMsIdleSubState
// DESCRIPTION :: MM sublayer substates in IDLE state in the mobile station
// **/
typedef enum
{
    UMTS_MM_MS_IDLE_NULL = 0,
    UMTS_MM_MS_IDLE_NORMAL_SERVICE = 1,
    UMTS_MM_MS_IDLE_ATTEMPTING_TO_UPDATE = 2,
    UMTS_MM_MS_IDLE_NO_CELL_AVAILABLE = 5,
    UMTS_MM_MS_IDLE_LOCATION_UPDATE_NEEDED = 6,
    UMTS_MM_MS_IDLE_PLMN_SEARCH = 7,
    UMTS_MM_MS_IDLE_PLMN_SEARCH_NORMAL_SERVICE = 8,
    UMTS_MM_MS_IDLE_RECEIVING_GROUP_CALL = 9
} UmtsMmMsIdleSubState;

// /**
// ENUM        :: UmtsMmUpdateStatus
// DESCRIPTION :: MM sublayer update status in the mobile station
// **/
typedef enum
{
    UMTS_MM_UPDATED = 1,
    UMTS_MM_NOT_UPDATED = 2,
    UMTS_MM_ROAMING_NOT_ALLOWED = 3
} UmtsMmUpdateStatus;

///////////
// GMM 
///////////
// /**
// ENUM        :: UmtsGmmMsState
// DESCRIPTION :: GMM sublayer main states in the mobile station
// **/
typedef enum
{
    UMTS_GMM_MS_NULL,
    UMTS_GMM_MS_DEREGISTERED,
    UMTS_GMM_MS_REGISTERED_INITIATED,
    UMTS_GMM_MS_REGISTERED,
    UMTS_GMM_MS_DEREGISTERED_INITIATED,
    UMTS_GMM_MS_ROUTING_AREA_UPDATING_INITIATED,
    UMTS_GMM_MS_SERVICE_REQUEST_INITIATED
} UmtsGmmMsState;

// /**
// ENUM        :: UmtsGmmMsDeregSubState
// DESCRIPTION :: GMM sublayer substates in the state DEREGISTERED
//                in the mobile station
// **/
typedef enum
{
    UMTS_GMM_MS_DEREGISTERED_NORMAL_SERVICE,
    UMTS_GMM_MS_DEREGISTERED_LIMITED_SERVICE,
    UMTS_GMM_MS_DEREGISTERED_ATTACH_NEEDED,
    UMTS_GMM_MS_DEREGISTERED_ATTEMPTING_TO_ATTACH,
    UMTS_GMM_MS_DEREGISTERED_NO_CELL_AVAILABLE,
    UMTS_GMM_MS_DEREGISTERED_PLMN_SEARCH
} UmtsGmmMsDeregSubState;

// /**
// ENUM        :: UmtsGmmMsRegSubState
// DESCRIPTION :: GMM sublayer substates in the state 
//                REGISTERED in the mobile station
// **/
typedef enum
{
    UMTS_GMM_MS_REGISTERED_NORMAL_SERVICE,
    UMTS_GMM_MS_REGISTERED_UPDATE_NEEDED, 
    UMTS_GMM_MS_REGISTERED_ATTEMPTING_TO_UPDATE,
    UMTS_GMM_MS_REGISTERED_NO_CELL_AVAILABLE,
    UMTS_GMM_MS_REGISTERED_ATTEMPTING_TO_UPDATE_MM,
    UMTS_GMM_MS_REGISTERED_IMSI_DETACH_INITIATED,
    UMTS_GMM_MS_REGISTERED_PLMN_SEARCH
} UmtsGmmMsRegSubState;

// /**
// ENUM        :: UmtsGMmUpdateStatus
// DESCRIPTION :: GMM sublayer update status in the mobile station
// **/
typedef enum
{
    UMTS_GMM_UPDATED = 1,
    UMTS_GMM_NOT_UPDATED = 2,
    UMTS_GMM_ROAMING_NOT_ALLOWED = 3
} UmtsGMmUpdateStatus;

// /**
// ENUM        :: UmtsCcMsState
// DESCRIPTION :: Call control substates in the mobile station
// **/
typedef enum
{
    UMTS_CC_MS_NULL = 0,
    UMTS_CC_MS_MM_CONNECTION_PENDING = 2,
    UMTS_CC_MS_CC_PROMPT_PRESENT = 34,
    UMTS_CC_MS_WAIT_FOR_NETWORK_INFORMATION = 35,
    UMTS_CC_MS_CC_ESTABLISHMENT_PRESENT = 36,
    UMTS_CC_MS_CC_ESTABLISHMENT_CONFIRMED = 37,
    UMTS_CC_MS_CC_RECALL_PRESENT = 38,
    UMTS_CC_MS_CALL_INITIATED = 1,
    UMTS_CC_MS_MOBILE_ORIGINATING_CALL_PROCEEDING = 3,
    UMTS_CC_MS_CALL_DELIVERED = 4,
    UMTS_CC_MS_CALL_PRESENT = 6,
    UMTS_CC_MS_CALL_RECEIVED = 7,
    UMTS_CC_MS_CONNECT_REQUEST = 8,
    UMTS_CC_MS_MOBILE_TERMINATING_CALL_CONFIRMED = 9,
    UMTS_CC_MS_ACTIVE = 10,
    UMTS_CC_MS_DISCONNECT_REQUEST = 11,
    UMTS_CC_MS_DISCONNECT_INDICATION = 12,
    UMTS_CC_MS_RELEASE_REQUEST = 19,
    UMTS_CC_MS_MOBILE_ORIGINATING_MODIFY = 26,
    UMTS_CC_MS_MOBILE_TERMINATING_MODIFY = 27
} UmtsCcMsState;

// /**
// ENUM        :: UmtsSmMsState
// DESCRIPTION :: Session management states in the mobile station
// **/
typedef enum
{
    UMTS_SM_MS_PDP_INACTIVE,
    UMTS_SM_MS_PDP_ACTIVE_PENDING,
    UMTS_SM_MS_PDP_INACTIVE_PENDING,
    UMTS_SM_MS_PDP_ACTIVE,
    UMTS_SM_MS_MODIFY_PENDING,
    UMTS_SM_MS_PDP_REJECTED,
} UmtsSmMsState;

// /**
// STRUCT      :: UmtsLayer3UeRrcConnReqInfo
// DESCRIPTION :: Information kept at UE during 
//                RRC CONNECTION REQUEST procedure
// **/
typedef struct
{
    UmtsLayer3CnDomainId cnDomainId;
} UmtsLayer3UeRrcConnReqInfo;

// /**
// STRUCT      :: UmtsLayer3UeCellInfo
// DESCRIPTION :: Information of a cell kept at the UE
// **/
typedef struct umts_layer3_ue_cell_info_str
{
} UmtsLayer3UeCellInfo;


// /**
// STRUCT      :: UmtsLayer3UePara
// DESCRIPTION :: Parameters of an UMTS UE
// **/
typedef struct umts_layer3_ue_para_str
{
    // parameters which control cell selection and reselection
    //double Qqualmin;        // minimum CPICH Ec/No for FDD
    double Qrxlevmin;       // minimum RX level for cell selection
    double Ssearch;         // Threshold to trigger cell search
    double Qhyst;           // Hysteresis for cell reselection
    clocktype cellEvalTime; // Interval for evaluate a cell
    double Qoffset1;        // offset between two cells cpich RSCP
    double Qoffset2;        // offset between two cells  CPICH Ec/No

    // parameters which control handover
    double shoAsTh;        // Threshold to add a cell into the AS
    double shoAsThHyst;    // Hysteresis for the AS threshold
    double shoAsRepHyst;   // Replacement hysteresis for AS
    int shoAsSizeMax;      // Maximum size of the AS set
    clocktype shoEvalTime; // Interval for evalulate a cell for SHO

} UmtsLayer3UePara;

// /**
// STRUCT      :: UmtsLayer3UeNodebInfo
// DESCRIPTION :: NodeB data of the cellular UMTS at UE
// **/
struct UmtsLayer3UeNodebInfo
{
    UInt32  cellId;
    UmtsPlmnIdentity plmnId;
    unsigned char valueTag;
    UmtsUeCellStatus cellStatus;
    UInt16 sibInd;
    UInt16 regAreaId;

    unsigned int sscCodeGroupIndex;
    unsigned int primaryScCode;     // the same as cell ID
    
    // canned for implemetation
    int ulChannelIndex;

    double cpichRscp; // CPICH RSCP
    double cpichEcNo; // CPICH Ec/No
    
    UmtsMeasTimeWindow* cpichRscpMeas;
    UmtsMeasTimeWindow* cpichEcNoMeas;
    // unsigned int cellId;
    
    // system frome number
    unsigned int sfnPccpch;

    // more system info
    
    UmtsLayer3UePara *hoCellSelectionParam;

    // params for PRACH
    UmtsPrachInfo *prachInfo;

    // param for AICh
    UmtsAichInfo *aichInfo;

    //param for PICH
    UmtsPichInfo *pichInfo;

    // param for PCCPCH
    UmtsPccpchInfo *pccpchInfo;

    //  param for SCCPCH
    UmtsSccpchInfo *sccpchInfo;

    // param for CPICH
    // UmtsCpichInfo cpichInfo;

    // params for PCH

    UmtsLayer3UeNodebInfo(int scCodeIndex, clocktype twSize): 
                                                primaryScCode(scCodeIndex)
    {
        sscCodeGroupIndex = scCodeIndex / 128;
        hoCellSelectionParam = NULL;
        cellId = (UInt32)primaryScCode;// cell Id is the same as priSccode
        prachInfo = NULL;
        aichInfo = NULL;
        pichInfo = NULL;
        pccpchInfo = NULL;
        sccpchInfo = NULL;
        valueTag = 0;
        sibInd = 0;
        cpichRscpMeas = new UmtsMeasTimeWindow(twSize);
        cpichEcNoMeas = new UmtsMeasTimeWindow(twSize);
        regAreaId = UMTS_INVALID_REG_AREA_ID;
    }
    ~UmtsLayer3UeNodebInfo()
    {
        delete cpichRscpMeas;
        delete cpichEcNoMeas;
    }
};

// /**
// STRUCT      :: UmtsLayer3UeStat
// DESCRIPTION :: UE specific statistics
// **/
typedef struct umts_layer3_ue_stat_str
{
    // cell selection and reselection related stats
    int numCellSearch;    // # of cell search performed
    int numCellInitSelection; // # of cell selection/reselection performed
    int numCellReselection;

    // system information
    int numMibRcvd;
    int numSib1Rcvd;
    int numSib2Rcvd;
    int numSib3Rcvd;
    int numSib4Rcvd;
    int numSib5Rcvd;

    // RR
    int numDlDirectTransferRcvd;
    int numRrcConnSetupRcvd; // # of RRC CONN SETUP rcvd
    int numRrcConnSetupCompSent; // # of RRC CONN SETUP COMPLETE sent
    int numRrcConnSetupRejRcvd; // # of RRC CONN SETUP REJECT
    int numRrcConnRelRcvd; // # of RRC CONN RELEASE rcvd
    int numPagingRcvd; // # of paging rcvd
    int numRadioBearerSetupRcvd; // # of RADIO BEARERSETUP rcvd
    int numRadioBearerRelRcvd; // # of RADIO BEARER RELEASE rcvd
    int numSigConnRelRcvd; // # of singalling Conn Rel rcvd
    int numSigConnRelIndSent; // # of signalling conn rel indication sent
    int numRrcConnReqSent; // # of RRC CONN REQ sent
    int numRrcConnRelCompSent; // # of RRC CONN REL COMPLETE send

    // MM
    int numLocUpReqSent;
    int numLocUpAcptRcvd;
    int numLocUpRejRcvd;
    
    // GMM
    int numAttachReqSent; // ATTACH REQUEST
    int numAttachAcptRcvd; // ATTACH ACCEPT
    int numAttachRejRcvd; // ATTACH REJECT
    int numAttachComplSent; // ATTACH COMPLETE 
    int numServiceReqSent; 
    int numServiceAcptRcvd;

    // SM
    int numActivatePDPContextReqSent; // ACITVATE PDP CONTEXT REQUEST
    int numActivatePDPContextAcptRcvd; // ACTIVATE PDP CONTEXT ACCEPT
    int numActivatePDPContextRejRcvd;  // ACTIVATE PDP CONTEXT REJECT
    int numRequestPDPConextActivationRcvd; // REQUEST PDP CONTEXT ACTIVATION
    int numDeactivatePDPContextReqSent; // DEACITVATE PDP CONTEXT REQUEST
    int numDeactivatePDPContextAcptRcvd; // DEACTIVATE PDP CONTEXT ACCEPT
    int numDeactivatePDPContextAcptSent;
    int numDeactivatePDPContextRequestRcvd; // DEACTIVATE PDP CONTEXT REQ
 
    // CC
    int numCcSetupSent;
    int numCcCallProcRcvd;
    int numCcAlertingRcvd;
    int numCcConnectRcvd;
    int numCcConnAckSent;
    int numCcDisconnectSent;
    int numCcReleaseRcvd;
    int numCcReleaseCompleteSent;

    int numCcSetupRcvd;
    int numCcCallConfirmSent;
    int numCcAlertingSent;
    int numCcConnectSent;
    int numCcConnAckRcvd;
    int numCcDisconnectRcvd;
    int numCcReleaseSent;
    int numCcReleaseCompleteRcvd;

    // handover related stats
    int numHardHoPeformed;
    int numSoftHoPerformed;
    int numSofterHoPerformed;
    int numIntraCellShoPerformed;
    int numInterCellIntraRncShoPerformed;
    int numInterRncIntraSgsnShoPerformed;
    int numInterSgsnShoPerformed;
    int numActiveSetSetupRcvd; // # of active setup rcvd
    int numActiveSetupCompSent; 

    // measurement 
    int numMeasComandRcvd;
    int numMeasReportSent;
    int numCpichMeasFromPhyRcvd;

    // For data packets
    int numNasMsgSentToL2;
    int numDataPacketFromUpper;
    int numDataPacketDroppedUnsupportedFormat;
    int numPacketRcvdFromL2;
    int numDataPacketRcvdFromL2;
    int numCtrlPacketRcvdFromL2;

    int numPsDataPacketFromUpper;
    int numPsDataPacketFromUpperEnQueue;
    int numPsDataPacketFromUpperDeQueue;
    int numPsDataPacketSentToL2;
    int numPsPktDroppedNoResource;    
                        // data packet dropped due to no resource

    int numPsDataPacketRcvdFromL2;
    
    int numCsDataPacketFromUpper;
    int numCsDataPacketFromUpperEnQueue;
    int numCsDataPacketFromUpperDeQueue;
    int numCsDataPacketSentToL2;
    int numCsPktDroppedNoResource;    
                        // data packet dropped due to no resource

    int numCsDataPacketRcvdFromL2;

    // MT application
    // CS speech
    // CS data
    // PS data
    int numMtAppRcvdFromNetwork;
    int numMtAppInited;
    int numMtAppRejected;
    int numMtAppDropped;
    int numMtAppSucEnded;

    int numMtConversationalAppRcvdFromNetwork;
    int numMtConversationalAppInited;
    int numMtConversationalAppRejected;
    int numMtConversationalAppDropped;
    int numMtConversationalAppSucEnded;

    int numMtStreamingAppRcvdFromNetwork;
    int numMtStreamingAppInited;
    int numMtStreamingAppRejected;
    int numMtStreamingAppDropped;
    int numMtStreamingAppSucEnded;

    int numMtInteractiveAppRcvdFromNetwork;
    int numMtInteractiveAppInited;
    int numMtInteractiveAppRejected;
    int numMtInteractiveAppDropped;
    int numMtInteractiveAppSucEnded;

    int numMtBackgroundAppRcvdFromNetwork;
    int numMtBackgroundAppInited;
    int numMtBackgroundAppRejected;
    int numMtBackgroundAppDropped;
    int numMtBackgroundAppSucEnded;

    int numMtCsAppRcvdFromNetwork;
    int numMtCsAppInited;
    int numMtCsAppRejected;
    int numMtCsAppDropped;
    int numMtCsAppSucEnded;

    // MO application
    // CS speech
    // CS data
    // PS data
    int numMoAppRcvdFromUpper;
    int numMoAppInited;
    int numMoAppRejected;
    int numMoAppDropped;
    int numMoAppSucEnded;

    int numMoConversationalAppRcvdFromUpper;
    int numMoConversationalAppInited;
    int numMoConversationalAppRejected;
    int numMoConversationalAppDropped;
    int numMoConversationalAppSucEnded;

    int numMoStreamingAppRcvdFromUpper;
    int numMoStreamingAppInited;
    int numMoStreamingAppRejected;
    int numMoStreamingAppDropped;
    int numMoStreamingAppSucEnded;

    int numMoInteractiveAppRcvdFromUpper;
    int numMoInteractiveAppInited;
    int numMoInteractiveAppRejected;
    int numMoInteractiveAppDropped;
    int numMoInteractiveAppSucEnded;

    int numMoBackgroundAppRcvdFromUpper;
    int numMoBackgroundAppInited;
    int numMoBackgroundAppRejected;
    int numMoBackgroundAppDropped;
    int numMoBackgroundAppSucEnded;

    int numMoCsAppRcvdFromUpper;
    int numMoCsAppInited;
    int numMoCsAppRejected;
    int numMoCsAppDropped;
    int numMoCsAppSucEnded;

    // dynamic Statistics
    int priNodeBCpichRscpGuiId;
    int priNodeBCpichEcNoGuiId;
    int priNodeBCpichBerGuiId;
    int activeSetSizeGuiId;
} UmtsLayer3UeStat;

//--------------------------------------------------------------------------
// main data structure for UMTS layer3 implementation of UE
//--------------------------------------------------------------------------
// /**
// STRUCT      :: UmtsRrcUeData
// DESCRIPTION :: RRC data structure at UE side
// **/
struct UmtsRrcUeData
{
    NodeId      ueId;                       //UE identifier, used for 
                                            //occasions for initial 
                                            //UE ID and RNTIs
    UInt32      ueScCode;                   //UE primary scrambling code
    UmtsRrcState    state;
    UmtsRrcSubState subState;
    UInt32 selectedCellId;
    
    BOOL        amRlcErrRb234;              
    BOOL        amRlcErrRb5up;
    BOOL        orderedReconfiguration;
    BOOL        cellUpStarted;
    BOOL        protocolErrorIndi;          //protocol error needed to 
                                            //report to UTRAN
    BOOL        protocolErrorRej;           //Protocol error cause procedure
                                            //to fail
    BOOL        failureIndi;                //Indicate whether a UE 
                                            //initiated proc failed
    UmtsRrcEstCause   estCause;             //Store RRC establishment CAUSE

    BOOL        rachOn;                     //whether random access channel
                                            //has been setup
    UmtsLayer3UeRrcConnReqInfo  rrcConnReqInfo;
    //Timer messages and counters
    Message*    timerT300;
    Message*    timerT308;
    Message*    timerRrcConnReqWaitTime;
    
    UInt8       counterV300;
    UInt8       counterV308;

    //Transcations
    std::map<UmtsRRMessageType, unsigned char>* acceptedTranst; 
    std::map<UmtsRRMessageType, unsigned char>* rejectedTranst; 
    
    //RAB and RB
    UmtsUeRabInfo*  rabInfos[UMTS_MAX_RAB_SETUP];  //established RAB info
    char    rbRabId[UMTS_MAX_RB_ALL_RAB];          //The RABID that each 
                                                   //RB belongs to
                                                   // -1 if the RB 
                                                   //is not setup 
    BOOL signalConn[UMTS_MAX_CN_DOMAIN];           //Established signal 
                                                   //connections

    //physical and transport channel parameters
};

// /**
// STRUCT      :: UmtsMmMsData
// DESCRIPTION :: Structure of the UE specific MM data in the MS side
// **/
typedef struct 
{
    UInt32 tmsi; 
    UmtsMmMsState           mainState;
    UmtsMmMsIdleSubState    idleSubState;
    UmtsMmUpdateStatus      updateState; 

    BOOL locUpdateNeeded;

    UmtsLocUpdateType nextLocUpType;

    UmtsMmConnCont*     actMmConns;     //active MM connections
    UmtsMmConnQueue*    rqtingMmConns; //Queued MM connections

    BOOL    pagingRcvd;
    //Message* T3211;
    Message* T3212;
    Message* T3230;
    Message* T3240;
    clocktype periodicUpInterval;
    
} UmtsMmMsData;

// /**
// STRUCT      :: UmtsGMmMsData
// DESCRIPTION :: Structure of the UE specific GMM data in the MS side
// **/
typedef struct 
{
    UmtsPmmState            pmmState;
    UmtsGmmMsState          mainState;
    UmtsGmmMsDeregSubState  deRegSubState;
    UmtsGmmMsRegSubState    regSubState;
    UmtsGMmUpdateStatus     updateState;

    //BOOL psSignConnEsted; // PS signalling connection established or not
    
    // attach/detach
    unsigned int attachAtmpCount; // attach attempt count
    unsigned int raUpdAtmpCount; // rouitng area update attempt count
    Message* T3310;
    UmtsAttachType nextAttachType;
    BOOL AttCompWaitRelInd;

    // servie request
    //BOOL servReqInited;
    BOOL servReqWaitForRrc;
    unsigned char serviceTypeRequested;
    unsigned char servReqTransId;

    Message* T3317;
    Message* T3319;

} UmtsGMmMsData;

// /**
// STRUCT      :: UmtsCcMsData
// DESCRIPTION :: Structure of the UE specific CC data 
// **/
typedef struct 
{
    UmtsCcMsState state;

    int    callAppId;
    NodeId peerAddr;
    clocktype endTime;
    clocktype avgTalkTime;

    Message* T303;
    Message* T310;
    Message* T313;
    Message* T305;
    Message* T308;
    char     counterT308;
} UmtsCcMsData;

// /**
// STRUCT      :: UmtsSmMsData
// DESCRIPTION :: Structure of the UE specific SM data 
// **/
typedef struct 
{
    UmtsSmMsState smState;
    BOOL  needSendActivateReq;
    
    Message* T3380;
    Message* T3390;
    unsigned char counterT3380;
    unsigned char counterT3390;
} UmtsSmMsData;

// /**
// STRUCT      :: UmtsCmMsData
// DESCRIPTION :: Structure of the UE call management 
// **/
typedef struct
{
    UmtsCcMsData* ccData;
    UmtsSmMsData* smData;
}UmtsCmMsData;

// /**
// STRUCT      :: UmtsLayer3UeAppInfo
// DESCRIPTION :: Structure of the application information at UE
// **/
struct UmtsLayer3UeAppInfo
{
    // if it is UE initiated, transId is from UE itself
    // it is network side initiated, transId from network side
    unsigned char transId;
    UmtsFlowSrcDestType srcDestType; 
    unsigned char nsapi;
    char rabId; // RAB id
    char rbId; // the rbId for this app
    clocktype lastActiveTime;

    // classfier associate with this application
    UmtsLayer3FlowClassifier* classifierInfo;

    // QoS info
    UmtsFlowQoSInfo* qosInfo;

    // pakcet buffer
    std::queue<Message*>* inPacketBuf;
	UInt32 pktBufSize;

    UmtsCmMsData cmData;

    UmtsLayer3UeAppInfo(unsigned char transCount,
                        UmtsFlowSrcDestType srcDest)
                         
    {
        transId = transCount;
        srcDestType = srcDest;
        classifierInfo = NULL;
        inPacketBuf = new std::queue<Message*>;
	    pktBufSize = 0;	
        qosInfo = NULL;
        rbId = UMTS_INVALID_RB_ID;
        rabId = UMTS_INVALID_RAB_ID;
        nsapi = 0;
        cmData.ccData = NULL;
        cmData.smData = NULL;
    }

    ~UmtsLayer3UeAppInfo()
    {
        delete inPacketBuf;
    }
};

// /**
// STRUCT      :: UmtsLayer3Ue
// DESCRIPTION :: Structure of the UE specific layer 3 data UMTS UE node
// **/
typedef struct struct_umts_layer3_ue_str
{
    CellularIMSI *imsi;      //UE's International Mobile Subscriber Identity
    UInt32 tmsi;

    unsigned int locRoutAreaId; //use RNC ID for convenience

    // some system config related info
    UmtsPlmnType plmnType;
    UmtsDuplexMode duplexMode;

    UmtsLayer3UePara para;  // important parameters
    UmtsLayer3UeStat stat;  // UE specific statistics

    // in/out service 
    UmtsUeServiceAreaStatus serivceAreaStatus;

    // UE's nodeB info
    std::list <UmtsLayer3UeNodebInfo*>* ueNodeInfo;
    UmtsLayer3UeNodebInfo* priNodeB;
    Message* Tselection;
    
    UmtsRrcUeData*  rrcData;
    UmtsMmMsData*   mmData;
    UmtsGMmMsData*  gmmData;

    // application information
    //unsigned char transIdCount; // for local initiated application only
    std::bitset<UMTS_MAX_NASPI>* nsapiBitSet;
    std::bitset<UMTS_MAX_TRANS_MOBILE>* transIdBitSet;
    std::list<UmtsLayer3UeAppInfo*>* appInfoList;
    unsigned char numOngoingPsApp;
    unsigned char numOngoingCsApp;
    unsigned char numOngoingApp;

    //measurement report
    clocktype lastReportTime;

    // HSDPA
    BOOL hsdpaEnabled;
    
} UmtsLayer3Ue;

//--------------------------------------------------------------------------
//  API functions
//--------------------------------------------------------------------------
// /**
// FUNCTION   :: UmtsLayer3UeHandlePacketFromUpper
// LAYER      :: Layer3
// PURPOSE    :: Handle pakcets from upper
// PARAMETERS ::
// + node             : Node*    : Pointer to node.
// + msg              : Message* : Message to be sent over the air interface
// + interfaceIndex   : int  : Interface from which the packet is received
// + networkType      : int  : network Type IPv4 or IPv6
// RETURN     :: void : NULL
// **/
void UmtsLayer3UeHandlePacketFromUpper(
            Node* node,
            Message* msg,
            int interfaceIndex,
            int networkType);
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
                                  UmtsLayer3Data*,
                                  NodeId ueId,
                                  char rbId);

// /**
// FUNCTION   :: UmtsLayer3UeReceivePacketFromMacLayer
// LAYER      :: Layer3
// PURPOSE    :: Handle timers and layer messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int  : Interface from which the packet is received
// + umtsL3           : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + msg              : Message*          : Message for node to interpret.
// + lastHopAddress   : NodeAddress       : Address of the last hop
// RETURN     :: void : NULL
// **/
void UmtsLayer3UeReceivePacketFromMacLayer(
    Node *node,
    int interfaceIndex,
    UmtsLayer3Data* umtsL3,
    Message *msg,
    NodeAddress lastHopAddress);


//  /** 
// FUNCITON   :: UmtsLayer3UeHandleInterLayerCommand
// LAYER      :: UMTS L3 at UE
// PURPOSE    :: Handle Interlayer command 
// PARAMETERS :: 
// + node             : Node*             : Pointer to node.
// + cmdType          : UmtsInterlayerCommandType : command type
// + interfaceIdnex   : UInt32            : interface index of UMTS
// + cmd              : void*          : cmd to handle
// RETURN     :: void : NULL
void UmtsLayer3UeHandleInterLayerCommand(
            Node* node,
            UInt32 interfaceIndex,
            UmtsInterlayerCommandType cmdType,
            void* cmd);
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
                      UmtsLayer3Data *umtsL3);

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
void UmtsLayer3UeLayer(Node *node, Message *msg, UmtsLayer3Data *umtsL3);

// /**
// FUNCTION   :: UmtsLayer3UeFinalize
// LAYER      :: Layer 3
// PURPOSE    :: Print UE specific stats and clear protocol variables.
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// RETURN     :: void : NULL
// **/
void UmtsLayer3UeFinalize(Node *node, UmtsLayer3Data *umtsL3);

#endif /* _LAYER3_UMTS_UE_H_ */
