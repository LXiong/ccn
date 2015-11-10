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

#ifndef _LAYER3_UMTS_GSN_H_
#define _LAYER3_UMTS_GSN_H_

#include <queue>
//using namespace std;

//--------------------------------------------------------------------------
// main data structure for UMTS layer3 implementation of GSN
//--------------------------------------------------------------------------

// /**
// ENUM        :: UmtsMmNwState
// DESCRIPTION :: MM sublayer main states in the network side
// **/
typedef enum
{
    UMTS_MM_NW_IDLE = 1,
    UMTS_MM_NW_WAIT_FOR_RR_CONNECTION = 2,
    UMTS_MM_NW_CONNECTION_ACTIVE = 3,
    UMTS_MM_NW_IDENTIFICATION_INITIATED = 4,
    UMTS_MM_NW_AUTHENTICATION_INITIATED = 5,
    UMTS_MM_NW_TMSI_REALLOCATION_INITIATED = 6,
    UMTS_MM_NW_SECURITY_MODE_INITIATED = 7,
    UMTS_MM_NW_WAIT_FOR_MOBILE_ORIGINATED_MM_CONNECTION = 81,
    UMTS_MM_NW_WAIT_FOR_NETWORK_ORIGINATED_MM_CONNECTION = 82,
    UMTS_MM_NW_WAIT_FOR_REESTABLISHMENT = 9,
    UMTS_MM_NW_WAIT_FOR_A_GROUP_CALL = 10,
    UMTS_MM_NW_GROUP_CALL_ACTIVE = 11,
    UMTS_MM_NW_MM_CONNECTION_ACTIVE_GROUP_CALL = 12,
    UMTS_MM_NW_WAIT_FOR_BROADCAST_CALL = 13,
    UMTS_MM_NW_BROADCAST_CALL_ACTIVE = 14
} UmtsMmNwState;

// /**
// ENUM        :: UmtsGMmNwState
// DESCRIPTION :: GMM sublayer main states in the network side
// **/
typedef enum
{
    UMTS_GMM_NW_DEREGISTERED,
    UMTS_GMM_NW_COMMON_PROCEDURE_INITIATED,
    UMTS_GMM_NW_REGISTERED,
    UMTS_GMM_NW_DEREGISTERED_INITIATED
} UmtsGmmNwState;

// /**
// ENUM        :: UmtsGmmNwRegSubState
// DESCRIPTION :: GMM sublayer substates in the state REGISTERED (NW side)
// **/
typedef enum
{
    UMTS_GMM_NW_REGISTERED_NORMAL_SERVICE,
    UMTS_GMM_NW_REGISTERED_NORMAL_SUSPENDED
} UmtsGmmNwRegSubState;

// /**
// ENUM        :: UmtsCcNwState
// DESCRIPTION :: Call control substates in the network side
// **/
typedef enum
{
    UMTS_CC_NW_NULL = 0,
    UMTS_CC_NW_CALL_INITIATED = 1,
    UMTS_CC_NW_MM_CONNECTION_PENDING = 2,
    UMTS_CC_NW_MOBILE_ORIGINATING_CALL_PROCEEDING = 3,
    UMTS_CC_NW_CALL_DELIVERED = 4,
    UMTS_CC_NW_CALL_PRESENT = 6,
    UMTS_CC_NW_CALL_RECEIVED = 7,
    UMTS_CC_NW_CONNECT_REQUEST = 8,
    UMTS_CC_NW_MOBILE_TERMINATING_CALL_CONFIRMED = 9,
    UMTS_CC_NW_ACTIVE = 10,
    UMTS_CC_NW_ACTIVE_PENDING_ON_RAB_EST = 11,
    UMTS_CC_NW_DISCONNECT_INDICATION = 12,
    UMTS_CC_NW_RELEASE_REQUEST = 19,
    UMTS_CC_NW_MOBILE_ORIGINATING_MODIFY = 26,
    UMTS_CC_NW_MOBILE_TERMINATING_MODIFY = 27,
    UMTS_CC_NW_CONNECT_INDICATION = 28,
    UMTS_CC_NW_CONN_INDI_PENDING_ON_RAB_EST = 29,
    UMTS_CC_NW_CC_CONNECTION_PENDING = 34,
    UMTS_CC_NW_NETWORK_ANSWER_PENDING = 35,
    UMTS_CC_NW_CC_ESTABLISHMENT_PRESENT = 36,
    UMTS_CC_NW_CC_ESTABLISHMENT_CONFIRMED = 37,
    UMTS_CC_NW_CC_RECALL_PRESENT = 38,
} UmtsCcNwState;

// /**
// ENUM        :: UmtsGmscCcState
// DESCRIPTION :: Call control states in the GMSC
// **/
typedef enum
{
    UMTS_CC_GMSC_NULL,
    UMTS_CC_GMSC_MOBILE_TERMINATING_ROUTING_PENDING,
    UMTS_CC_GMSC_CALL_INITIATED,
    UMTS_CC_GMSC_CALL_PRESENT,
    UMTS_CC_GMSC_CALL_CONFIRMED,
    UMTS_CC_GMSC_ALERTING,
    UMTS_CC_GMSC_ACTIVE,
} UmtsGmscCcState;

// /**
// ENUM        :: UmtsSmNwState
// DESCRIPTION :: Session management states in the network side
// **/
typedef enum
{
    UMTS_SM_NW_PDP_INACTIVE,
    UMTS_SM_NW_PDP_ACTIVE_PENDING,
    UMTS_SM_NW_PDP_INACTIVE_PENDING,
    UMTS_SM_NW_PDP_ACTIVE,
    UMTS_SM_NW_MODIFY_PENDING,
    UMTS_SM_NW_PAGE_PENDING,
    UMTS_SM_NW_PDP_REJECTED,
} UmtsSmNwState;

// /**
// ENUM        :: UmtsLayer3GgsnPdpStatus
// DESCRIPTION :: Status of a PDP context at the GGSN
// **/
typedef enum
{
    UMTS_GGSN_PDP_INIT,               // initial, not req routing info yet
    UMTS_GGSN_PDP_ROUTING_PENDING,    // sent query to HLR for routing info
    UMTS_GGSN_PDP_PDU_NOTIFY_PENDING, // send PDU notification req to SGSN
    UMTS_GGSN_PDP_PDU_NOTIFIED,       // received PDU notifiction response
    UMTS_GGSN_PDP_ACTIVE,             // PDP context is active
    UMTS_GGSN_PDP_REJECTED,           // The PDP activation failed, we wait
                                      // some time before deleting it
} UmtsLayer3GgsnPdpStatus;

// /**
// STRUCT      :: UmtsMmNwData
// DESCRIPTION :: Structure of the network MM data for an UE
// **/
typedef struct
{
    UmtsMmNwState           mainState;
    UmtsMmConnCont*         actMmConns;
    //UmtsMmConnQueue*        rqtingMmConns;

    Message*                timerT3255;
} UmtsMmNwData;

// /**
// STRUCT      :: UmtsGMmNwData
// DESCRIPTION :: Structure of the UE specific GMM data in the network side
// **/
typedef struct
{
    UmtsPmmState            pmmState;
    UmtsGmmNwState          mainState;
    UmtsGmmNwRegSubState    regSubState;
    //BOOL psSignConnEsted; // PS signalling connection established or not
    // attach/detach

    BOOL followOnProc;     //Follow On proceed
    Message* T3350;
    Message* T3313;
    unsigned char counterT3350;
    char     counterT3313;

    // routing area
} UmtsGMmNwData;

// /**
// STRUCT      :: UmtsCcNwData
// DESCRIPTION :: Structure of the NW specific CC data
// **/
typedef struct
{
    UmtsCcNwState state;

    NodeId peerAddr;
    int    callAppId;
    clocktype endTime;
    clocktype avgTalkTime;

    // Timer
    Message* T301;
    Message* T303;
    Message* T310;
    Message* T313;
    Message* T305;
    Message* T308;
    char     counterT308;
} UmtsCcNwData;

// /**
// STRUCT      :: UmtsSmNwData
// DESCRIPTION :: Structure of the NW specific SM data
// **/
typedef struct
{
    UmtsSmNwState smState;

    // Timer
    Message* T3385;
    Message* T3395;

    char     counterT3385;
    char     counterT3395;
} UmtsSmNwData;

// /**
// STRUCT      :: UmtsCmNwData
// DESCRIPTION :: Structure of the NW call management
// **/
typedef struct
{
    UmtsCcNwData* ccData;
    UmtsSmNwData* smData;
}UmtsCmNwData;

// /**
// STRUCT      :: UmtsRabAsgntTimerInfo
// DESCRIPTION :: RAB assignment timer info
// **/
struct UmtsRabAsgntTimerInfo
{
    NodeId ueId;
    int    teId;
};

// /**
// STRUCT      :: UmtsLayer3SgsnFlowInfo
// DESCRIPTION :: Structure of the application information at SGSN
// **/
struct UmtsLayer3SgsnFlowInfo
{
    // if it is UE initiated, transId is from UE itself
    // it is network side initiated, transId from network side
    unsigned char transId;
    UmtsFlowSrcDestType srcDestType;
    UInt32 ueId;

    unsigned char nsapi;
    UInt32 ulTeId; // tunneling Id, generated by GGSN, used by RNC/BSC
    UInt32 dlTeId; // tunneling Id, generated by SGSN, used by GGSN

    char rabId; // RAB id
    char rbId; // the rbId for this flow
    NodeId rncId;
    NodeId ggsnNodeId; // My primary GGSN's node ID
    Address ggsnAddr;  // My primary GGSN's network address

    // classifier associate with this application
    UmtsLayer3FlowClassifier* classifierInfo;

    // QoS info
    UmtsFlowQoSInfo* qosInfo;

    // packet buffer
    std::queue <Message*>* inPacketBuf;

    UmtsCmNwData cmData;

    UmtsLayer3SgsnFlowInfo(unsigned char transCount,
                           UmtsFlowSrcDestType srcDest)

    {
        transId = transCount;
        srcDestType = srcDest;
        classifierInfo = NULL;
        inPacketBuf = new std::queue<Message*>;
        qosInfo = NULL;
        rbId = UMTS_INVALID_RB_ID;
        rabId = UMTS_INVALID_RAB_ID;
        cmData.ccData = NULL;
        cmData.smData = NULL;
        ulTeId = UMTS_INVALID_TE_ID;
        dlTeId = UMTS_INVALID_TE_ID;
        ueId = 0;

    }
    ~UmtsLayer3SgsnFlowInfo()
    {
        delete inPacketBuf;
    }
};

// /**
// STRUCT      :: UmtsLayer3GgsnPdpInfo
// DESCRIPTION :: Structure of the application information at SGSN
// **/
struct UmtsLayer3GgsnPdpInfo
{
    UmtsLayer3GgsnPdpStatus status;  // status of the PDP context
    UmtsFlowSrcDestType srcDestType; // Mobile inited or network inited
    clocktype lastActiveTime;        // last time the flow has activity

    NodeId ueId;
    Address uePdpAddr;   // IP address of the UE
    CellularIMSI imsi;   // IMSI of the UE, stores home MCC and MNC
    UInt32 tmsi;         // TMSI PTMSI assigned

    unsigned char nsapi;
    UInt32 ulTeId; // tunneling Id, generated by GGSN, used by RNC/BSC
    UInt32 dlTeId; // tunneling Id, generated by SGSN, used by GGSN

    NodeId sgsnId;
    Address sgsnAddr; //assume data and signalling use the same SGSN

    // classifier associate with this application
    UmtsLayer3FlowClassifier* classifierInfo;

    // QoS info
    UmtsFlowQoSInfo* qosInfo;

    // packet buffer
    std::queue <Message*>* inPacketBuf;
	UInt32 pktBufSize;

    UmtsLayer3GgsnPdpInfo()
    {
        status = UMTS_GGSN_PDP_INIT;
        srcDestType = UMTS_FLOW_MOBILE_ORIGINATED;
        lastActiveTime = 0;
        classifierInfo = NULL;
        ueId = 0;
        memset(&uePdpAddr, 0, sizeof(uePdpAddr));
        memset(&imsi, 0 , sizeof(imsi));
        tmsi = 0;
        nsapi = 0;

        inPacketBuf = new std::queue<Message*>;
		pktBufSize = 0;
        qosInfo = NULL;
        ulTeId = UMTS_INVALID_TE_ID;
        dlTeId = UMTS_INVALID_TE_ID;

        sgsnId = 0;
        memset(&sgsnAddr, 0, sizeof(sgsnAddr));
    }
    ~UmtsLayer3GgsnPdpInfo()
    {
        delete inPacketBuf;
    }
};

// /**
// STRUCT      :: UmtsLayer3GmscCallInfo
// DESCRIPTION :: Structure of the voice call info at GMSC
// **/
struct UmtsLayer3GmscCallInfo
{
    UmtsGmscCcState state;
    NodeId ueId;
    char   ti;
    int    callAppId;
    NodeId peerAddr;
    Address mscAddr;
    clocktype endTime;
    clocktype avgTalkTime;

    UmtsLayer3GmscCallInfo(
        NodeId ue,
        char transId,
        NodeId peer,
        int appId,
        clocktype ts_end,
        clocktype avgTalkDur,
        const Address* msc)
        : ueId(ue), ti(transId), callAppId(appId), peerAddr(peer),
          endTime(ts_end), avgTalkTime(avgTalkDur)
    {
        if (msc)
        {
            memcpy(&mscAddr, msc, sizeof(Address));
        }
        else
        {
            memset(&mscAddr, 0, sizeof(Address));
        }
        state = UMTS_CC_GMSC_NULL;
    }
};

typedef std::list<UmtsLayer3GmscCallInfo*> UmtsGmscCallFlowList;

// /**
// STRUCT      :: UmtsSgsnUeDataInSgsn
// DESCRIPTION :: UE data structure per UE at SGSN
// ** /
struct UmtsSgsnUeDataInSgsn
{
    NodeId          ueId;
    CellularIMSI imsi;   // IMSI of the UE, stores home MCC and MNC
    //TMSI PTMSI assigned
    UInt32 tmsi;
    UInt32 rncId;        // node ID of current RNC
    unsigned char transIdCount;
    std::bitset<UMTS_MAX_TRANS_MOBILE>* transIdBitSet;

    // MM
    UmtsMmNwData* mmData;

    // GMM
    UmtsGMmNwData* gmmData;

    // application info
    std::list<UmtsLayer3SgsnFlowInfo*>* appInfoList;
};

// /**
// STRUCT      :: UmtsVlrEntry
// DESCRIPTION :: One entry which stores information of a UE in the VLR
// **/
typedef struct struct_umts_vlr_entry_str
{
    // following are two keys
    UInt32 hashKey;      // Hash key which is UE ID, uniquelly global

    // basic info of the UE
    CellularIMSI imsi;   // IMSI of the UE, stores home MCC and MNC
    UInt32 tmsi;         // TMSI
    UInt32 pmsi;         // P-TMSI
    UInt32 lmci;         // LMSI

    // information of serving PLMN network
    UmtsRoutingAreaId rai; // Routing Area Identification (RAI)
                           // Rai.lac is the cell (NodeB) id
    UInt32 rncId;        // node ID of current RNC
} UmtsVlrEntry;

typedef std::map<UInt32, UmtsVlrEntry*> UmtsVlrMap;

// /**
// STRUCT      :: UmtsLayer3SgsnStat
// DESCRIPTION :: SGSN specific statistics
// **/
typedef struct umts_layer3_sgsn_stat_str
{
    // PS data packets
    int numTunnelDataPacketFwdToGgsn;  // GTP tunnelled data pkt to GGSN
    int numTunnelDataPacketFwdToUe;    // GTP tunnelled data pkt to UE
    int numTunnelDataPacketDropped;    // GTPtunnelled data pkt dropped

    // CS data packets
    int numCsPktsToGmsc;
    int numCsPktsToUe;
    int numCsPktsDropped;

    // MM

    // GMM
    int numRoutingUpdateRcvd;     // Routing Area Update from UE
    int numRoutingUpdateAcptSent; // Routing Area Update Accept to UE
    int numRoutingUpdateRjtSent;  // Routing Area Update Reject to UE
    int numAttachReqRcvd;         // Attach Request from UE
    int numAttachAcptSent;        // Attach Accept to UE
    int numAttachRejSent;         // Attach Reject to UE
    int numAttachComplRcvd;       // Attach Complete from UE
    int numPagingSent;            // Paging message to UE
    int numServiceReqRcvd;        // Service Request from UE
    int numServiceAcptSent;       // Service Accept to UE
    int numServiceRejSent;        // Service Reject to UE

    // SM with UE
    // PDP Context Activation
    int numActivatePDPContextReqRcvd; 
                                  // Activate PDP Context Request from UE
    int numActivatePDPContextAcptSent;   
                                  // Activate PDP Context Accept to UE
    int numActivatePDPContextRjtSent;    
                                  // Activate PDP Context Reject to UE
    int numReqActivatePDPContextSent;    
                                  // Request PDP Context Activation to UE

    // PDP Context Modification
    int numModifyPDPContextReqRcvd;      
                                 // Modify PDP Context Request from UE
    int numModifyPDPContextAcptSent;     
                                 // Modify PDP Context Accept to UE
    int numModifyPDPContextRjtSent;      
                                 // Modify PDP Context Reject to UE
    int numModifyPDPContextReqSent;      
                                 // Modify PDP Context Request to UE
    int numModifyPDPContextAcptRcvd;    
                                 // Modify PDP Context Accept from UE

    // PDP Context Deactivation
    int numDeactivatePDPContextReqRcvd;  
                                 // Deactivate PDP Context Request from UE
    int numDeactivatePDPContextAcptSent; 
                                 // Deactivate PDP Context Accept to UE
    int numDeactivatePDPContextReqSent;  
                                 // Deactivate PDP Context Request to UE
    int numDeactivatePDPContextAcptRcvd; 
                                 // Deactivate PDP Context Accept from UE


    // PDP Context with GGSN
    // Create PDP Context
    int numPduNotificationReqRcvd;   // PDU Notification Request from GGSN
    int numPduNotificationRspSent;   // PDU Notification Response to GGSN
    int numCreatePDPContextReqSent;  // Create PDP Context Request to GGSN
    int numCreatePDPContextRspRcvd;  
                                // Create PDP Context Response from GGSN

    // Update PDP Context
    int numUpdatePDPContextReqSent;  // Update PDP Context Request to GGSN
    int numUpdatePDPContextRspRcvd;  
                                // Update PDP Context Response from GGSN
    int numUpdatePDPContextRejRcvd;  // Update PDP Context Reject from GGSN
    int numUpdatePDPContextReqRcvd;  // Update PDP Context Request from GGSN
    int numUpdatePDPContextRspSent;  // Update PDP Context Response to GGSN

    // Delete PDP Context
    int numDeletePDPContextReqSent;  // Delete PDP Context Request to GGSN
    int numDeletePDPContextRspRcvd;  
                                 // Delete PDP Context Response from GGSN
    int numDeletePDPContextReqRcvd;  // Delete PDP Context Request from GGSN
    int numDeletePDPContextRspSent;  // Delete PDP Context Response to GGSN

    // CC
    int numCcSetupSent;
    int numCcCallConfirmRcvd;
    int numCcAlertingRcvd;
    int numCcConnectRcvd;
    int numCcConnAckSent;
    int numCcDisconnectSent;
    int numCcReleaseRcvd;
    int numCcReleaseCompleteSent;

    int numCcSetupRcvd;
    int numCcCallProcSent;
    int numCcAlertingSent;
    int numCcConnectSent;
    int numCcConnAckRcvd;
    int numCcDisconnectRcvd;
    int numCcReleaseSent;
    int numCcReleaseCompleteRcvd;

    // VLR and HLR
    int numVlrEntryAdded;            // Added routing/location info to VLR
    int numVlrEntryRemoved;          // Removed entry from VLR
    int numHlrUpdate;                // Send update to HLR

    // dyanmic stats
    int numRegedUeGuiId;
} UmtsLayer3SgsnStat;

// /**
// STRUCT      :: UmtsLayer3GgsnStat
// DESCRIPTION :: GGSN specific statistics
// **/
typedef struct umts_layer3_ggsn_stat_str
{
    // flow statistics
    // Mobile Terminated Flows
    // Aggregated flow statistics
    int numMtFlowRequested;    // Mobile terminated flows requested
    int numMtFlowAdmitted;     // Mobile terminated flows admitted
    int numMtFlowRejected;     // Mobile terminated flows rejected
    int numMtFlowDropped;      // Mobile terminated flows dropped
    int numMtFlowCompleted;    // Mobile terminated flows completed

    // Conversational flows
    int numMtConversationalFlowRequested;
    int numMtConversationalFlowAdmitted;
    int numMtConversationalFlowRejected;
    int numMtConversationalFlowDropped;
    int numMtConversationalFlowCompleted;

    // Streaming flows
    int numMtStreamingFlowRequested;
    int numMtStreamingFlowAdmitted;
    int numMtStreamingFlowRejected;
    int numMtStreamingFlowDropped;
    int numMtStreamingFlowCompleted;

    // Interactive flows
    int numMtInteractiveFlowRequested;
    int numMtInteractiveFlowAdmitted;
    int numMtInteractiveFlowRejected;
    int numMtInteractiveFlowDropped;
    int numMtInteractiveFlowCompleted;

    // Background flows
    int numMtBackgroundFlowRequested;
    int numMtBackgroundFlowAdmitted;
    int numMtBackgroundFlowRejected;
    int numMtBackgroundFlowDropped;
    int numMtBackgroundFlowCompleted;

    // Mobile Originated Flows
    // Aggregated flow statistics
    int numMoFlowRequested;    // Mobile originated flows requested
    int numMoFlowAdmitted;     // Mobile originated flows admitted
    int numMoFlowRejected;     // Mobile originated flows rejected
    int numMoFlowDropped;      // Mobile originated flows dropped
    int numMoFlowCompleted;    // Mobile originated flows completed

    // Conversational flows
    int numMoConversationalFlowRequested;
    int numMoConversationalFlowAdmitted;
    int numMoConversationalFlowRejected;
    int numMoConversationalFlowDropped;
    int numMoConversationalFlowCompleted;

    // Streaming flows
    int numMoStreamingFlowRequested;
    int numMoStreamingFlowAdmitted;
    int numMoStreamingFlowRejected;
    int numMoStreamingFlowDropped;
    int numMoStreamingFlowCompleted;

    // Interactive flows
    int numMoInteractiveFlowRequested;
    int numMoInteractiveFlowAdmitted;
    int numMoInteractiveFlowRejected;
    int numMoInteractiveFlowDropped;
    int numMoInteractiveFlowCompleted;

    // Background flows
    int numMoBackgroundFlowRequested;
    int numMoBackgroundFlowAdmitted;
    int numMoBackgroundFlowRejected;
    int numMoBackgroundFlowDropped;
    int numMoBackgroundFlowCompleted;

    // aggregated data packet statistics
    int numPsDataPacketFromOutsidePlmn;  // PS data pkts from outside
    int numPsDataPacketToPlmn;           // PS data pkts routed to my PLMN
    int numPsDataPacketFromPlmn;         // PS data pkts from my PLMN
    int numPsDataPacketToOutsidePlmn;    // PS data pkts routed to outside
    int numPsDataPacketDropped;          // PS data pkts dropped
    int numPsDataPacketDroppedUnsupportedFormat;

    int numCsPktsFromOutside;
    int numCsPktsToInside;
    int numCsPktsFromInside;
    int numCsPktsToOutside;
    int numCsPktsDropped;

    // PDP Context
    // PDP Context Activation by network
    int numPduNotificationReqSent;   // PDU Notification Request to SGSN
    int numPduNotificationRspRcvd;   // PDU Notification Response from SGSN
    // PDP Context Activation by UE or network
    int numCreatePDPContextReqRcvd;  // Create PDP Context Request from SGSN
    int numCreatePDPContextRspSent;  // Create PDP Context Response to SGSN

    // PDP Context Modification by UE
    int numUpdatePDPContextReqRcvd;  // Update PDP Context Request from SGSN
    int numUpdatePDPContextRspSent;  // Update PDP Context Response to SGSN
    int numUpdatePDPContextRejSent;  // Update PDP Context Reject to SGSN
    // PDP Context Modification by network
    int numUpdatePDPContextReqSent;  
                                   // Update PDP Context Request to SGSN
    int numUpdatePDPContextRspRcvd;  
                                   // Update PDP Context Response from SGSN

    // PDP Context Deactivation by UE
    int numDeletePDPContextReqRcvd;  // Delete PDP Context Request from SGSN
    int numDeletePDPContextRspSent;  // Delete PDP Context Response to SGSN
    // PDP Context Deactivation by network
    int numDeletePDPContextReqSent;  // Delete PDP Context Request to SGSN
    int numDeletePDPContextRspRcvd;  
                                    // Delete PDP Context Response from SGSN

    // HLR
    int numRoutingQuerySent;         // Queries to HLR for routing info
    int numRoutingQueryReplyRcvd;    // Replies from HLR for routing info
    int numRoutingQueryFailRcvd;     // Replies from HLR with failure status

    // danamic stats
#if 0
    int numIngressConn;
    int numEgressConn;
    int numEgressConnGuiId;
    int numIngressConnGuiId;
#endif

    int egressThruputGuiId;
    int ingressThruputGuiID;

    UmtsMeasTimeWindow* egressThruputMeas;
    UmtsMeasTimeWindow* ingressThruputMeas;
} UmtsLayer3GgsnStat;

// /**
// STRUCT      :: UmtsUeLocCacheEntry
// DESCRIPTION :: UE location cache entry
// **/
struct UmtsUeLocCacheEntry
{
    CellularMCC mcc;    // MCC of current area
    CellularMNC mnc;    // MNC of current area
    NodeId sgsnId;      // Current serving SGSN node
    Address sgsnAddr;   // Address of SGSN node

    clocktype ts_lastAccess; //last access time

    explicit UmtsUeLocCacheEntry(Node* node,
                                const UmtsUeLocCacheEntry* ueLoc)
    {
        memcpy(&mcc, &(ueLoc->mcc), sizeof(mcc));
        memcpy(&mnc, &(ueLoc->mnc), sizeof(mnc));
        sgsnId = ueLoc->sgsnId;
        memcpy(&sgsnAddr, &(ueLoc->sgsnAddr), sizeof(sgsnAddr));
        ts_lastAccess = node->getNodeTime();
    }
};

// /**
// TYPEDEF     :: UmtsUeLocEntryMap
// DESCRIPTION :: UE Location entry MAP, cached at GGSN(GMSC)
// **/
typedef std::map<NodeId, UmtsUeLocCacheEntry*> UmtsUeLocEntryMap;

// /**
// STRUCT      :: UmtsLayer3Gsn
// DESCRIPTION :: Structure of the GSN specific layer 3 data UMTS GSN node
// **/
typedef struct struct_umts_layer3_gsn_str
{
    BOOL isGGSN;  // Gateway GSN
    UInt32 gsnId; // ID of the GSN node

    NodeId hlrNodeId;  // Node ID of the HLR of my PLMN
    Address hlrAddr;   // Address of the HLR of my PLMN

    NodeId ggsnNodeId; // My primary GGSN's node ID
    Address ggsnAddr;  // My primary GGSN's network address

    UmtsVlrMap* vlrMap;

    // list of RNCs connected to me
    LinkedList *rncList;
    LinkedList *addrInfoList;

    // UE current under the SGSN's control
    std::list <UmtsSgsnUeDataInSgsn*>* ueInfoList;
    std::list <UmtsLayer3GgsnPdpInfo*>* pdpInfoList;
    UmtsGmscCallFlowList* callFlowList;

	UInt64 agregPktBufSize;     //aggregated packet buffer size

    // GTP
    UInt32 teIdCount; // both SGSN and GGSN has its own counter

    UmtsUeLocEntryMap* ueLocCacheMap;
    UmtsCellCacheMap* cellCacheMap;     //CELL ID/Cached info map
    UmtsRncCacheMap*  rncCacheMap;     //CELL ID/Cached info map

    UmtsLayer3SgsnStat sgsnStat;    // SGSN specific statistics
    UmtsLayer3GgsnStat ggsnStat;    // GGSN specific statistics

    Message* flowTimeoutTimer;  // timer msg for detecting flow timeout
} UmtsLayer3Gsn;


//--------------------------------------------------------------------------
//  API functions
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
                               Address srcAddr);

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
         BOOL internal = FALSE);

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
                       UmtsLayer3Data *umtsL3);

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
void UmtsLayer3GsnLayer(Node *node, Message *msg, UmtsLayer3Data *umtsL3);

// /**
// FUNCTION   :: UmtsLayer3GsnFinalize
// LAYER      :: Layer 3
// PURPOSE    :: Print GSN specific stats and clear protocol variables.
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// RETURN     :: void : NULL
// **/
void UmtsLayer3GsnFinalize(Node *node, UmtsLayer3Data *umtsL3);

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
                                     int netType);
#endif /* _LAYER3_UMTS_GSN_H_ */
