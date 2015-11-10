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

#ifndef _LAYER3_UMTS_RNC_H_
#define _LAYER3_UMTS_RNC_H_

#include <bitset>
#include <list>
#include <map>
#include <set>
#include <queue>
#include <vector>

#include "umts_constants.h"

//--------------------------------------------------------------------------
// RNC's specific constants
//--------------------------------------------------------------------------

// /**
// CONSTANT    :: UMTS_RNC_DEF_SYSTEM_INFO_BROADCAST_INTERVAL : 30S
// DESCRIPTION :: Default value of the interval between system info msgs
// **/
#define UMTS_RNC_DEF_SYSTEM_INFO_BROADCAST_INTERVAL (30 * SECOND)

// /**
// CONSTANT    :: UMTS_RNC_DEF_SHO_AS_THRESHOLD : 3.0 dBm
// DESCRIPTION :: Default value of the AS threshold for Soft Handover
// **/
#define UMTS_RNC_DEF_SHO_AS_THRESHOLD (3.0)

// /**
// CONSTANT    :: UMTS_RNC_DEF_SHO_AS_THRESHOLD_HYSTERESIS : 1.0 dBm
// DESCRIPTION :: Default value of the hysteresis of SHO AS Threshold
// **/
#define UMTS_RNC_DEF_SHO_AS_THRESHOLD_HYSTERESIS (1.0)

// /**
// CONSTANT    :: UMTS_RNC_DEF_SHO_AS_REPLACE_HYSTERESIS : 1.0 dBm
// DESCRIPTION :: Default value of the replace hysteresis for SHO
// **/
#define UMTS_RNC_DEF_SHO_AS_REPLACE_HYSTERESIS (1.0)


// /**
// CONSTANT    :: UMTS_RNC_MAX_SPREAD_FACOR : 256
// DESCRIPTION :: Maximum spending factor of the code. This must be
//                power of 2.
// **/
#define UMTS_RNC_MAX_SPREAD_FACTOR     256

// /**
// CONSTANT    :: UMTS_RNC_MAX_SPREAD_FACOR_LEVEL : 8
// DESCRIPTION :: Level of maximum spread factor in the code tree. This one
//                must be consistent with UMTS_RNC_MAX_SPREAD_FACOR.
//                This limits the alloc. of largest spread factor of code.
//                For example, 8 means system can only allocate a code
//                with spread factor equal to or smaller than 2^8 = 256.
// **/
#define UMTS_RNC_MAX_SPREAD_FACTOR_LEVEL     8

// /**
// CONSTANT    :: UMTS_RNC_MIN_SPREAD_FACOR_LEVEL : 2
// DESCRIPTION :: Level of minimum spread factor in the code tree. This one
//                limits the allocation of smallest spread factor of code.
//                For example, 2 means system can only allocate a code
//                with spread factor equal to or larger than 2^2 = 4
// **/
#define UMTS_RNC_MIN_SPREAD_FACTOR_LEVEL     2

// /**
// CONSTANT    :: UMTS_RNC_MAX_HSDSCH_CODE   5
// DESCRIPTION :: The maximum hs-dsch code supported in the system
// **/
#define UMTS_RNC_MAX_HSDSCH_CODE 5

// /**
// CONSTANT    :: UMTS_RNC_MAX_NUM_CODE_PER_ALLOC : 15
// DESCRIPTION :: Max number of codes can be allocated once
// **/
#define UMTS_RNC_MAX_NUM_CODE_PER_ALLOC     15

// /**
// CONSTANT    :: UMTS_RNC_MAX_PRIM_CELL_SWITCH_DURATION
// DESCRIPTION :: Maximum duration allowed to switch a primary cell
// **/
const clocktype UMTS_RNC_MAX_PRIM_CELL_SWITCH_DURATION = 80*MILLI_SECOND;

//--------------------------------------------------------------------------
// RNC's specific enums
//--------------------------------------------------------------------------

// /**
// ENUM        :: UmtsLayer3RncCodeStaus
// DESCRIPTION :: Status of a code in the code tree. Used for allocation
// **/
typedef enum umts_layer3_rnc_code_status
{
    UMTS_CODE_FREE = 0,      // code is free for allocation
    UMTS_CODE_ALLOCATED = 1, // code has been allocated
    UMTS_CODE_RESERVED = 2,  // code has been reserved, cannot be used
    UMTS_CODE_RESERVED_HSDPA = 3,
} UmtsLayer3RncCodeStatus;

//--------------------------------------------------------------------------
// RNC's specific data structures
//--------------------------------------------------------------------------

// /**
// STRUCT      :: UmtsMntNodebInfo
// DESCRIPTION :: Monitored NodeB information for a UE
// **/
struct UmtsMntNodebInfo
{
    UInt32  cellId;
    double dlRscpMeas;
    clocktype timeStamp;
    UmtsUeCellStatus cellStatus;
    //double ulMeas;
    //clocktype lastUlReportTime;
    UmtsMntNodebInfo(UInt32 cell)
                    : cellId(cell)
    {
        dlRscpMeas = UMTS_MIN_FDD_CELL_SELECTION_MIN_RX_LEVEL - 1;
        cellStatus = UMTS_CELL_MONITORED_SET;
        timeStamp = 0;
    }
    bool operator < (const UmtsMntNodebInfo& cmpVar) const
    {
        return dlRscpMeas > cmpVar.dlRscpMeas;
    }
};

#ifdef _WIN64
template<>
struct std::greater<UmtsMntNodebInfo*>  : 
    public binary_function<UmtsMntNodebInfo* ,UmtsMntNodebInfo*, bool>
{
    bool operator()(const UmtsMntNodebInfo* x, 
                    const UmtsMntNodebInfo* y) const
    {
        if (*x < *y)
        {
            return TRUE;
        }
        else if (*y < *x)
        {
            return FALSE;
        }
        else
        {
            return FALSE;
        }
    };
};
#endif

// /**
// STRUCT      :: UmtsLayer3RncPara
// DESCRIPTION :: Parameters of an UMTS RNC
// **/
typedef struct umts_layer3_rnc_para_str
{
    // parameters which control scheduling of system information

    // parameters which control handover
    double shoAsTh;        // Threshold to add a cell into the AS
    double shoAsThHyst;    // Hysteresis for the AS threshold
    double shoAsRepHyst;   // Replacement hysteresis for AS
    int shoAsSizeMax;      // Maximum size of the AS set
    clocktype shoEvalTime; // Interval for evalulate a cell for SHO
} UmtsLayer3RncPara;

// /**
// STRUCT      :: UmtsLayer3RncStat
// DESCRIPTION :: RNC specific statistics
// **/
typedef struct umts_layer3_rnc_stat_str
{
    // data packet related statistics
    int numDataPacketFromUe;
    int numDataPacketFromUeDroppedNoRrc;
    int numDataPacketFromUeDroppedNoRab;
    int numPsDataPacketToSgsn;
    int numCsDataPacketToSgsn;

    int numDataPacketFromSgsn;
    int numDataPacketFromSgsnDroppedNoRrc;
    int numDataPacketFromSgsnDroppedNoRab;
    int numPsDataPacketToUe;
    int numCsDataPacketToUe;

    int numRrcConnReqRcvd; // # of RRC CONN REQ rcvd
    int numRrcConnSetupSent; // # of RRC CONN SETUP sent
    int numRrcConnSetupCompRcvd; // # of RRC CONN SETUP COMPLETE rcvd
    int numRrcConnSetupRejSent; // # of RRC CONN SETUP REJECT sent

} UmtsLayer3RncStat;

//--------------------------------------------------------------------------
// main data structure for UMTS layer3 implementation of RNC
//--------------------------------------------------------------------------
// /**
// STRUCT      :: UmtsUlDchInfo
// DESCRIPTION :: Uplink DCH info for each UE
// **/
struct  UmtsUlDchInfo
{
    UInt32 rbBitSet;                        //RB bitset
};

// /**
// STRUCT      :: UmtsDlDchInfo
// DESCRIPTION :: Downlink DCH info for each NodeB
// **/
struct  UmtsDlDchInfo
{
    unsigned int chId;
    //NodeId       ueId;
    UInt32       rbBitSet;                  //RB bitset
};

// /**
// STRUCT      :: UmtsUlDpdchInfo
// DESCRIPTION :: Uplink DPDCH info for each UE
// **/
struct  UmtsUlDpdchInfo
{
    UInt32 rbBitSet;                        //RB bitset
};

// /**
// STRUCT      :: UmtsDlDpdchInfo
// DESCRIPTION :: Downlink DPDCH info for each NodeB
// **/
struct  UmtsDlDpdchInfo
{
    unsigned int     chId;
    // channel type can be DPDCH or HS-PDSCH
    UmtsPhysicalChannelType chType;
    UmtsSpreadFactor sfFactor;
    unsigned int     dlChCode;
    UInt32           rbBitSet;              //RB bitset
};

// /**
// STRUCT      :: UmtsDlHsscchInfo
// DESCRIPTION :: Downlink HS-SCCH info for each NodeB
// **/
struct  UmtsDlHsscchInfo
{
    unsigned int     chId;
    UmtsSpreadFactor sfFactor;
    unsigned int     dlChCode;
};

// /**
// STRUCT      :: UmtsRrcPendingRabInfo
// DESCRIPTION :: RAB info in pending state(to setup or release)
// **/
struct  UmtsRrcPendingRabInfo
{
    UmtsUeRbSetupPara uePara;
    int     numNodebRes;
    std::list<UmtsRrcNodebResInfo*>* nodebResList;

    UmtsRrcPendingRabInfo(const UmtsUeRbSetupPara& ueConfig)
                         : uePara(ueConfig), numNodebRes(0)
    {
        nodebResList = new std::list<UmtsRrcNodebResInfo*>;
    }

    ~UmtsRrcPendingRabInfo()
    {
        std::list<UmtsRrcNodebResInfo*>::iterator iter;
        for (iter = nodebResList->begin();
             iter != nodebResList->end(); ++iter)
        {
            delete (*iter);
        }
        delete nodebResList;
    }
};

// /**
// STRUCT      :: UmtsCellSwitchQuePkt
// DESCRIPTION :: RRC data structure per UE at RNC
// **/
struct UmtsCellSwitchQuePkt
{
    Message* msg;
    char     rbId;

    UmtsCellSwitchQuePkt(Message* pkt, char rb)
                        : msg(pkt), rbId(rb)
    { 
    }

    ~UmtsCellSwitchQuePkt()
    {
    }
};

// /**
// STRUCT      :: UmtsRrcReleaseReq
// DESCRIPTION :: RRC Connection Release Request related inforamtion
// **/
struct UmtsRrcReleaseReq
{
    BOOL requested;     //Whether request has been initiated
    BOOL networkInit;   //Whether request is initiated by core network.
};

// /**
// STRUCT      :: UmtsRrcUeHsdpaInfo
// DESCRIPTION :: RRC UE Info for HSDPA
// **/
struct UmtsRrcUeHsdpaInfo
{
   unsigned char numHsdpaRb;
};

// /**
// STRUCT      :: UmtsRrcUeDataInRnc
// DESCRIPTION :: RRC data structure per UE at RNC
// **/
struct UmtsRrcUeDataInRnc
{
    NodeId          ueId;          // UE identifier, used for occasions for 
                                   // initial UE ID and RNTIs
    UInt32          primScCode;    // UE primary scrambling code

    NodeId          primCellId;    //  The primary NodeB Id for the UE
    NodeId          prevPrimCellId; // The Pri. NodeBId to be switched from.
    BOOL            primCellSwitch; // whether switching is taking place
#if 0
    int             cellSwitchFragSum; // Total number of cell switch 
                                       // fragments expected
    int             cellSwitchFragNum; // The number of cell switch 
                                       // fragments currently received
#endif // Old cell switch code

    std::vector<UmtsCellSwitchQuePkt*>*  cellSwitchQueue;   
                                       // Packet queue during cell switch

    //BOOL            releasePending;  // RRC CONN Release pending
    UmtsRrcState    state;
    UmtsRrcSubState subState;
    char            defaultConfigId;   // default configuration ID

    clocktype       tsLastMeasRep;     // Time stamp of last measurement 
                                       // report received.

    //Timer messages and counters
    Message*    timerT3101;
    Message*    timerMeasureCheck;

    //Transcations
    std::map<UmtsRRMessageType, unsigned char>* transactions;

    //downlink DPDCH info for pending setup of SRBs at UE side
    UmtsRcvPhChInfo* pendingDlDpdchInfo;

    //RAB and RB
    UmtsRrcRabInfo*  rabInfos[UMTS_MAX_RAB_SETUP];  //established RAB info
    UmtsRrcPendingRabInfo* pendingRab[UMTS_MAX_RAB_SETUP];
    char  rbRabId[UMTS_MAX_RB_ALL_RAB]; // The RABID that each RB belongs to
                                        // -1 if the RB is not setup
    BOOL signalConn[UMTS_MAX_CN_DOMAIN]; // Established signal connections

    //RB and RLC configurations
    std::list<UmtsRbMapping*>* rbMappingInfo;  // RB mapping info list, UL
    std::list<UmtsRbPhChMapping*>* rbPhChMappingList; // RB phy. channel 
                                                      // mapping info, uplink

    //DCCH DTCH Logical channel bitset
    std::bitset<UMTS_MAX_LOCH>* ulDcchBitSet;
    std::bitset<UMTS_MAX_LOCH>* ulDtchBitSet;

    //Uplink DCH channel info
    UmtsUlDchInfo* ulDchInfo[UMTS_MAX_TRCH];

    //Uplink DPDCH info
    UmtsUlDpdchInfo* ulDpdchInfo[UMTS_MAX_DPDCH_UL];

    std::list<UmtsMntNodebInfo*>* mntNodebList;

    // bw req
    UInt32 curBwReq;
    UmtsSpreadFactor curSf;
    unsigned char numUlPhCh;

    UmtsRrcReleaseReq rrcRelReq;

    // HSDPA
    BOOL hsdpaEnabled;
    UmtsRrcUeHsdpaInfo hsdpaInfo;
};

// /**
// STRUCT      :: UmtsRrcUeHsdpaInfo
// DESCRIPTION :: RRC UE Info for HSDPA
// **/
struct UmtsRrcNodebHsdpaInfo
{
   unsigned char maxHsdpschSupport;
   unsigned char numHspdschAlloc;
   unsigned int hspdschCodeIndex[15];

   // assume only one HS-SCCH  allocated
   unsigned char numHsscchAlloc;
   unsigned int hsscchCodeIndex[15];

   int numHsdpaRb;
   int totalHsBwReq;
   int totalHsBwAllo;

   int bwReqPerPrio[UMTS_MAX_APP_CLASS_TYPE];
};

// /**
// STRUCT      :: UmtsRrcNodebDataInRnc
// DESCRIPTION :: NodeB data in Rnc
// **/
struct UmtsRrcNodebDataInRnc
{
    NodeId          nodebId;
    UInt32          cellId; //Used for primary scrambling code

    // HSDPA realted
    BOOL            hsdpaEnabled;
    UmtsRrcNodebHsdpaInfo hsdpaInfo;


    //Timer messages and counters

    //Transcations

    //RB and RLC configurations
    std::list<UmtsRbMapping*>*     rbMappingInfo;     
                           // RB mapping info list, downlink
    std::list<UmtsRbPhChMapping*>* rbPhChMappingList; 
                           // RB physical channel mapping info, downlink

    //DCCH DTCH Logical channel bitset
    std::bitset<UMTS_MAX_LOCH_PER_CELL>* dlDcchBitSet;
    std::bitset<UMTS_MAX_LOCH_PER_CELL>* dlDtchBitSet;

    //DCH Transport channel info and bitset
    std::list<UmtsDlDchInfo*>*  dlDchInfo;
    std::bitset<UMTS_MAX_DL_TRCH_PER_CELL>* dlDchBitSet;

    //DPDCH physical channel info and bitset
    // HS-PDSCH is  also included in these structure
    std::list<UmtsDlDpdchInfo*>*  dlDpdchInfo;
    std::bitset<UMTS_MAX_DL_DPCH_PER_CELL>* dlDpdchBitSet;

    // HS-SCCH channel info
    std::list<UmtsDlHsscchInfo*>* dlHsscchInfo;

    // Tree for code allocation at this nodeB.
    // Since this is a perfect binary tree, an array is used for imp.
    char codeTree[2 * UMTS_RNC_MAX_SPREAD_FACTOR - 1];

    // Total Ul BW Requested/allocated
    int totalUlBwReq;
    int totalUlBwAlloc;
    int peakUlBwAlloc;
    clocktype lastUlResUpdateTime;
    double ulResUtil;

    // total Dl BW requested/allocated
    int totalDlBwReq;
    int totalDlBwAlloc;
    int peakDlBwAlloc;
    clocktype lastDlResUpdateTime;
    double dlResUtil;
};

// /**
// STRUCT      :: UmtsSrncInfo
// DESCRIPTION :: Serving RNC information
// **/
struct UmtsSrncInfo
{
    NodeId srncId;
    unsigned int refCount;  //reference count
};

typedef std::map<NodeId, UmtsSrncInfo*> UmtsUeSrncMap;

// /**
// STRUCT      :: UmtsRncRabAssgtReqInfo
// DESCRIPTION :: RAB assignment request info at RNC
struct UmtsRncRabAssgtReqInfo
{
    NodeId ueId;
    char rabId;
    UmtsLayer3CnDomainId domain;
    UmtsRABServiceInfo ulRabPara;
    UmtsRABServiceInfo dlRabPara;
    UmtsRabConnMap connMap;
    UmtsRncRabAssgtReqInfo(NodeId reqUeId,
                           UmtsLayer3CnDomainId reqDomain,
                           UmtsRABServiceInfo& reqUlRabPara,
                           UmtsRABServiceInfo& reqDlRabPara,
                           UmtsRabConnMap& reqConnMap,
                           char rab = UMTS_INVALID_RAB_ID)
    {
        ueId = reqUeId;
        rabId = rab;
        domain = reqDomain;
        ulRabPara = reqUlRabPara;
        dlRabPara = reqDlRabPara;
        connMap = reqConnMap;
    }

    bool operator < (const UmtsRncRabAssgtReqInfo& cmpPara) const
    {
        //CS domain has higher priority
        if (domain != cmpPara.domain)
        {
            return domain < cmpPara.domain;
        }
        //otherwise,
        //The RAB request with higher traffic class has higher priority
        else
        {
            UmtsQoSTrafficClass thisClass = (UmtsQoSTrafficClass)0;
            UmtsQoSTrafficClass cmpClass = (UmtsQoSTrafficClass)0;

            if (ulRabPara.maxBitRate > 0 && dlRabPara.maxBitRate > 0)
            {
                thisClass = (ulRabPara.trafficClass <
                             dlRabPara.trafficClass) ?
                             ulRabPara.trafficClass : 
                             dlRabPara.trafficClass;
            }
            else if (ulRabPara.maxBitRate > 0 )
            {
                thisClass = ulRabPara.trafficClass;
            }
            else if (dlRabPara.maxBitRate > 0)
            {
                thisClass = dlRabPara.trafficClass;
            }
            else
            {
                ERROR_ReportError(
                    "wrong RAB assignment request parameters.");
            }

            if (cmpPara.ulRabPara.maxBitRate > 0
                && cmpPara.dlRabPara.maxBitRate > 0)
            {
                cmpClass = (cmpPara.ulRabPara.trafficClass
                                < cmpPara.dlRabPara.trafficClass)
                           ? cmpPara.ulRabPara.trafficClass
                           : cmpPara.dlRabPara.trafficClass;
            }
            else if (cmpPara.ulRabPara.maxBitRate > 0)
            {
                cmpClass = cmpPara.ulRabPara.trafficClass;
            }
            else if (cmpPara.dlRabPara.maxBitRate > 0)
            {
                cmpClass = cmpPara.dlRabPara.trafficClass;
            }
            else
            {
                ERROR_ReportError(
                    "wrong RAB assignment request parameters.");
            }

            return thisClass < cmpClass;
        }
        //otherwise, the first in, first serve. This is accomplished by
        //stable sort of STL list
    }
};

typedef std::list<UmtsRncRabAssgtReqInfo*> UmtsRabAssgtReqList;

#ifdef _WIN64
template<>
struct std::greater<UmtsRncRabAssgtReqInfo*>  : 
            public binary_function<UmtsRncRabAssgtReqInfo* ,
                                   UmtsRncRabAssgtReqInfo*, bool>
{
    bool operator()(const UmtsRncRabAssgtReqInfo* lPtr, 
                    const UmtsRncRabAssgtReqInfo* rPtr) const
    {
        if (*lPtr < *rPtr)
        {
            return TRUE;
        }
        else if (*rPtr < *lPtr)
        {
            return FALSE;
        }
        else
        {
            return FALSE;
        }
    };
};
#endif

// /**
// STRUCT      :: UmtsLayer3Rnc
// DESCRIPTION :: Structure of the RNC specific layer 3 data UMTS RNC node
// **/
typedef struct struct_umts_layer3_rnc_str
{
    UmtsLayer3RncPara para;
    UmtsLayer3RncStat stat;

    // list neighboring nodes reflecting net connectivity
    LinkedList *nodebList;      // my NodeBs
    LinkedList *neighRncList;   // neighb RNCs
    LinkedList *addrInfoList;   // address info allocated under this RNC
    UmtsLayer3GsnInfo sgsnInfo; // my GGSN

    UmtsCellCacheMap* cellCacheMap;     
                 //CELL ID / Cached info map, for looking up RNC 
                 //information of a NodeB quickly
    UmtsUeSrncMap*    ueSrncMap; //UE/SERVING RNC map, 
                                 //for looking up SRC of a UE.

    std::list<UmtsRrcUeDataInRnc*>* ueRrcs;
    std::list<UmtsRrcNodebDataInRnc*>* nodebRrcs;

    UmtsAdmissionControlType ulCacType; // UL call admission control method
    UmtsAdmissionControlType dlCacType; // DL call admission control method

    // queue for the incoming RAB assignement request
    UmtsRabAssgtReqList* rabServQueue;
} UmtsLayer3Rnc;

//--------------------------------------------------------------------------
//  API functions
//--------------------------------------------------------------------------

// /**
// FUNCTION   :: UmtsLayer3RncHandlePacket
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
void UmtsLayer3RncHandlePacket(Node *node,
                               Message *msg,
                               UmtsLayer3Data *umtsL3,
                               int interfaceIndex,
                               Address srcAddr);

// /**
// FUNCTION   :: UmtsLayer3RncInit
// LAYER      :: Layer 3
// PURPOSE    :: Initialize RNC data at UMTS layer 3 data.
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + nodeInput : const NodeInput* : Pointer to node input.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// RETURN     :: void : NULL
// **/
void UmtsLayer3RncInit(Node *node,
                       const NodeInput *nodeInput,
                       UmtsLayer3Data *umtsL3);

// /**
// FUNCTION   :: UmtsLayer3RncLayer
// LAYER      :: Layer 3
// PURPOSE    :: Handle RNC specific timers and layer messages.
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + msg       : Message*         : Message for node to interpret
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// RETURN     :: void : NULL
// **/
void UmtsLayer3RncLayer(Node *node, Message *msg, UmtsLayer3Data *umtsL3);

// /**
// FUNCTION   :: UmtsLayer3RncFinalize
// LAYER      :: Layer 3
// PURPOSE    :: Print RNC specific stats and clear protocol variables.
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// RETURN     :: void : NULL
// **/
void UmtsLayer3RncFinalize(Node *node, UmtsLayer3Data *umtsL3);

#endif /* _LAYER3_UMTS_RNC_H_ */
