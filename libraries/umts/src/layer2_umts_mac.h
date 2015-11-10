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

#ifndef CELLUAR_UMTS_LAYER2_MAC_H
#define CELLUAR_UMTS_LAYER2_MAC_H

#include <utility>
#include <deque>
#include <list>
#include <deque>
#include <vector>
#include <set>

#include "cellular.h"
#include "cellular_umts.h"

#ifdef PARALLEL //Parallel
#include "parallel.h"
#endif //endParallel

// /**
// CONSTANT   :: UMTS_MAC_RACH_T2_DEFAULT_TIME
// DESCRPTION :: RACH procedure T2 interval
// **/
#define UMTS_MAC_RACH_T2_DEFAULT_TIME     10* MILLI_SECOND

// /**
// ENUM:: UmtsMacEntityType
// DESCRIPTION :: UMTS MAC Entity Type
// Reference   :: 3GPP TS 25.321
// **/
typedef enum
{
    UMTS_MAC_b = 0,    // MAC-b Entity
    UMTS_MAC_cshm,     // MAC-b/sh/m entity
    UMTS_MAC_d,        // MAC-d Entity
    UMTS_MAC_hs,       // MAC-hs Entity
    UMTS_MAC_m,        // MAC-m entity
    UMTS_MAC_ees,      // MAC-e/es entity
}UmtsMacEntityType;

// /**
// STRUCT:: UmtsMacStats
// DESCRIPTION:: RLC sublayer stat structure at a UE & NodeB
// **/
typedef struct
{
    // to add mac stats here
} UmtsMacStats;

// /**
// STRUCT:: UmtsLogChInfo
// DESCRIPTION:: Umts logical Channel Info
// **/
struct UmtsLogChInfo
{
    NodeAddress ueId; // to distighush UE at NodeB
    UmtsLogicalChannelType logChType;
    UmtsChannelDirection logChDir;
    unsigned char logChId;
    unsigned char logChPrio;


    // TODO
    // priority

    UmtsLogChInfo(NodeAddress ue,
                  UmtsLogicalChannelType chType,
                  UmtsChannelDirection chDir,
                  unsigned char chId,
                  unsigned char chPrio)
    {
        ueId = ue;
        logChType = chType;
        logChDir = chDir;
        logChId = chId;
        logChPrio = chPrio;
    }
    bool operator < (const UmtsLogChInfo& cmpEntity) const
    {
        if (ueId < cmpEntity.ueId)
        {
            return true;
        }
        else if (ueId > cmpEntity.ueId)
        {
            return false;
        }
        else if (logChType < cmpEntity.logChType)
        {
            return true;
        }
        else if (logChType > cmpEntity.logChType)
        {
            return false;
        }
        else if (logChId < cmpEntity.logChId)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
};

// /**
// STRUCT      :: UmtsLogChInfoPtrLess
// DESCRIPTION :: Function struct to compare to UmtsLogChInfo
// **/
struct UmtsLogChInfoPtrLess {
    bool operator() (const UmtsLogChInfo* lEntity,
        const UmtsLogChInfo* rEntity) const
    {
        return *lEntity < *rEntity;
    }
};

// /**
// DESCRIPTION :: Reload std:great to support the smae function of
//                UmtsLogChInfoPtrLess on WIN 64 PL
// **/
#ifdef _WIN64
template<>
struct std::greater<UmtsLogChInfo*>  : 
    public binary_function<UmtsLogChInfo* ,UmtsLogChInfo*, bool>
{
    bool operator()(const UmtsLogChInfo* x, const UmtsLogChInfo* y) const
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
// STRUCT:: UmtsTransChInfo
// DESCRIPTION:: Umts transport Channel Info
// **/
struct UmtsTransChInfo
{
    NodeAddress ueId; // to distighush UE at Nodeb
    UmtsTransportChannelType trChType;
    UmtsChannelDirection trChDir;
    unsigned char trChId;
    unsigned int TTI; // in slot
    unsigned int slotCounter;

    // used to order the transport channel
    // only apply to DCH
    char logPrio;
    double   trChWeight;
    UInt64 buffLen;
    UmtsTransportFormat transFormat;

    UmtsTransChInfo(NodeAddress ue,
                    UmtsTransportChannelType chType,
                    UmtsChannelDirection chDir,
                    unsigned char chId,
                    unsigned int tti,
                    char logChPrio)
    {
        ueId = ue;
        trChType = chType;
        trChDir = chDir;
        trChId = chId;
        TTI = tti;
        slotCounter = 0;
        logPrio = logChPrio;
        trChWeight = 0.0;
        buffLen =0;
    }

    void SetTrChTti(unsigned int tti)
    {
        TTI = tti;
    }

    unsigned int GetTrChTti()
    {
        return TTI;
    }

    void UpdateTransFormat(UmtsTransportFormat format)
    {
        memcpy((void*)&transFormat,
                (void*)&format,
                sizeof(UmtsTransportFormat));
        TTI = format.TTI;

    }

    bool operator < (const UmtsTransChInfo& cmpEntity) const
    {
        if (logPrio < cmpEntity.logPrio)
        {
            return true;
        }
        else if (logPrio > cmpEntity.logPrio)
        {
            return false;
        }
        else if (trChWeight > cmpEntity.trChWeight)
        {
            return true;
        }
        else if (trChWeight < cmpEntity.trChWeight)
        {
            return false;
        }
        else if (ueId < cmpEntity.ueId)
        {
            return true;
        }
        else if (ueId > cmpEntity.ueId)
        {
            return false;
        }
        else if (trChType < cmpEntity.trChType)
        {
            return true;
        }
        else if (trChType > cmpEntity.trChType)
        {
            return false;
        }
        else if (trChId < cmpEntity.trChId)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
};

// /**
// STRUCT      :: UmtsTransChInfoPtrLess
// DESCRIPTION :: Function struct to compare to UmtsTransChInfo
// **/
struct UmtsTransChInfoPtrLess {
    bool operator() (const UmtsTransChInfo* lEntity,
        const UmtsTransChInfo* rEntity) const
    {
        return *lEntity < *rEntity;
    }
};

// /**
// DESCRIPTION :: Reload std:great to support the smae function of
//                UmtsTransChInfoPtrLess on WIN 64 PL
// **/
#ifdef _WIN64
template<>
struct std::greater<UmtsTransChInfo*>  :
    public binary_function<UmtsTransChInfo* ,UmtsTransChInfo*, bool>
{
    bool operator()(const UmtsTransChInfo* x, const UmtsTransChInfo* y) const
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

typedef std::list<UmtsLogChInfo*> UeLogChList;
typedef std::list<UmtsLogChInfo*> NodeBLogChList;

typedef std::set<UmtsLogChInfo*, UmtsLogChInfoPtrLess> NodeBLogChSet;
typedef std::list<UmtsTransChInfo*> UeTrChList;
typedef std::list<UmtsTransChInfo*> NodeBTrChList;
typedef std::set<UmtsTransChInfo*, UmtsTransChInfoPtrLess> NodeBTrChSet;

// /**
// STRUCT      :: UmtsRbLogTrPhMapping
// DESCRIPTION :: Logical channel.Transport channel, physical channel
//               mapping info for a radio bearer
// **/
struct UmtsRbLogTrPhMapping
{
    unsigned char rbId;

    UmtsLogicalChannelType ulLogChType;
    unsigned char ulLogChId;
    UmtsTransportChannelType ulTrChType;
    unsigned char ulTrChId;
    UmtsPhysicalChannelType ulPhChType;
    unsigned char ulPhChId; // should be a list, i.e., 
                            // std::list<unsigned char> ulPhChId;

    UmtsLogicalChannelType dlLogChType;
    unsigned char dlLogChId;
    UmtsTransportChannelType dlTrChType;
    unsigned char dlTrChId;
    UmtsPhysicalChannelType dlPhChType;
    unsigned char dlPhChId; // should be a list, i.e.,
                            // std::list<unsigned char> dlPhChId;

    // transport format currently use
    UmtsTransportFormat transFormat;
};

// /**
// STRUCT      :: UmtsMacEntity_bInfo
// DESCRIPTION :: MAC-b entity info
// **/
typedef struct
{
    // more info goes here
}UmtsMacEntity_bInfo;

// /**
// STRUCT:: UmtsMacEntityDInfo
// DESCRIPTION:: MAC-d entity info
// **/
typedef struct
{
    // more info goes here
}UmtsMacEntity_dInfo;

// /**
// STRUCT      :: UmtsMacEntityRachConfigInfo
// DESCRIPTION :: MAC rach entity config info
// **/
typedef struct
{
    // rach info  from RRC configuration
    unsigned char Mmax; // Maximum number of preamble cycles
    unsigned char NB01min; // Sets lower bound for random back-off
    unsigned char NB01max; // Sets upper bound for random back-off
    std::list<UmtsRachAscInfo*>* prachPartition; //ASC info
}UmtsMacEntityRachConfigInfo;

// /**
// ENUM       :: UmtsMacRachState
// DESCRPTION :: Umts Mac Rach State machine
// **/
typedef  enum
{
    UMTS_MAC_RACH_IDLE = 0,
    UMTS_MAC_RACH_PERSISTENCE_TEST,
    UMTS_MAC_RACH_WAIT_T2,
    UMTS_MAC_RACH_BACKOFF,
    UMTS_MAC_RACH_WAIT_AICH,
    UMTS_MAC_RACH_WAIT_TX,
}UmtsMacRachState;

// /**
// ENUM       :: UmtsMacRachStateTransitEvent
// DESCRPTION :: Umts Mac Rach State machine transition event
// **/
typedef  enum
{
    UMTS_MAC_RACH_EVENT_DataToTx = 0,
    UMTS_MAC_RACH_EVENT_MaxRetry,
    UMTS_MAC_RACH_EVENT_T2Expire,
    UMTS_MAC_RACH_EVENT_TestFailed,
    UMTS_MAC_RACH_EVENT_PhyAccessCnfNoAichRsp,
    UMTS_MAC_RACH_EVENT_TboExpire,
    UMTS_MAC_RACH_EVENT_PhyAccessCnfNack,
    UMTS_MAC_RACH_EVENT_TestPassedAccessReqSent,
    UMTS_MAC_RACH_EVENT_PhyDataReqSent,
    UMTS_MAC_RACH_EVENT_PhyAccessCnfAck,
    UMTS_MAC_RACH_EVENT_Invalid
}UmtsMacRachStateTransitEvent;

// /**
// STRUCT     :: UmtsMacEntityRachDynamicInfo
// DESCRIPTION:: MAC rach entity config info
// **/
typedef struct
{
    // rach info  from RRC configuration
    unsigned char retryCounter; // counter of preamble cycles
    //unsigned char minBO; // active lower bound for random back-off
    //unsigned char maxBO; // active upper bound for random back-off
    unsigned char curBo;
    double persistFactor;

    unsigned char activeAscIndex;

    unsigned char sigStartIndex;
    unsigned char sigEndIndex;
    UInt16 assignedSubCh;

    // timer
    Message* timerT2;
    Message* timerBO;

    // state variable
    UmtsMacRachState state;
}UmtsMacEntityRachDynamicInfo;

// /**
// STRUCT:: UmtsMacEntity_cshmInfo
// DESCRIPTION:: MAC-b entity info
// **/
typedef struct
{
    // RACH info
    UmtsMacEntityRachConfigInfo* rachConfig;

    // dynamic variable
    UmtsMacEntityRachDynamicInfo* activeRachInfo;
}UmtsMacEntity_cshmInfo;

// /**
// STRUCT:: UmtsMacUeData
// DESCRIPTION:: MAC sublayer structure at a UE
// **/
struct UmtsMacUeData
{
    // list of downlink channels in the PLMN
    // UE will scan these channels for searching the NodeB.
    std::vector<int> *dlChannelList;
    int numDLChannels;

    // for channel scan to attach
    // if it found signal it will stop
    // increase the index, and keep on receving
    // CPICH with this freq
    int scanIndex;

    // Logical channel Info, UL
    UeLogChList* ueLogChList;

    // transport channel Info, UL
    UeTrChList* ueTrChList;

    std::list <UmtsRbLogTrPhMapping*>* ueLogTrPhMap;
    std::deque<Message*>* txBurstBuf;    //Transmission PDU buffer
    std::deque<Message*>* rxBurstBuf;    // reception PDU buffer

    // MAC enetity
    UmtsMacEntity_cshmInfo* cshmEntity;
    UmtsMacEntity_dInfo* dEntity;
    UmtsMacEntity_bInfo* bEntity;

    // UE stat
    // TODO
};

// /**
// STRUCT:: UmtsMacNodeBUeInfo
// DESCRIPTION:: UE info stored at a NodeB
// **/
struct UmtsMacNodeBUeInfo
{
    NodeId ueId;
    unsigned int priSc;
    std::list<UmtsRbLogTrPhMapping*>* ueLogTrPhMap;
    std::deque<Message*>* txBurstBuf;    //Transmission PDU buffer
    std::deque<Message*>* rxBurstBuf;    // reception PDU buffer
};

// /**
// STRUCT:: UmtsMacNodeBData
// DESCRIPTION:: MAC sublayer structure at a NodeB
// **/
typedef struct
{

    // Logical channel Info DL
    NodeBLogChList* nodeBLogChList;

    // transport channel Info DL
    NodeBTrChList* nodeBTrChList;

    // store the mapping for the common logical channel
    // transport channel for this cell
    std::list <UmtsRbLogTrPhMapping*>* nodeBLocalMap;
    std::deque<Message*>* commonTxBurstBuf;    //Transmission PDU buffer
    std::deque<Message*>* commonRxBurstBuf;    // reception PDU buffer

    // store the info for each registered UE, in which
    // store the mapping  for the dedicated logical  and transport
    // channel for this registered UE
    std::list <UmtsMacNodeBUeInfo*>* macNodeBUeInfo;

    // nodeB stat
}UmtsMacNodeBData;

// /**
// STRUCT:: UmtsMacData
// DESCRIPTION:: MAC sublayer structure at a UE & NodeB
// **/
typedef struct
{
    void* myMacCellularData;

    // some physical info
    UmtsChipRateType chipRateType;
    UmtsDuplexMode duplexMode;
    BOOL hspdaEnabled;

    // current uplink and downlink channel
    int currentULChannel;
    int currentDLChannel;

    // slot time
    clocktype chipDuration;
    clocktype slotDuration;
    Message* umtsMacSlotTimer;
    unsigned char slotIndex;

    UmtsMacNodeBData* nodeBMacVar;
    UmtsMacUeData* ueMacVar;

    // stat
    UmtsMacStats stats;

#ifdef PARALLEL
    LookaheadHandle lookaheadHandle;
#endif
} UmtsMacData;

//--------------------------------------------------------------------------
//  API functions
//--------------------------------------------------------------------------

//  /**
// FUNCITON   :: UmtsMacHandleInterLayerCommand
// LAYER      :: UMTS L2 MAC
// PURPOSE    :: Handle Interlayer command
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIdnex   : UInt32            : interface index of UMTS
// + cmdType          : UmtsInterlayerCommandType : command type
// + cmd              : void*          : cmd to handle
// RETURN     :: void : NULL
void UmtsMacHandleInterLayerCommand(
            Node* node,
            UInt32 interfaceIndex,
            UmtsInterlayerCommandType cmdType,
            void* cmd);


// /**
// FUNCTION   :: UmtsMacInit
// LAYER      :: UMTS LAYER2 MAC Sublayer
// PURPOSE    :: Initialize UMTS MAC sublayer at a given interface.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : Interface index
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
void UmtsMacInit(Node* node,
                 UInt32 interfaceIndex,
                 const NodeInput* nodeInput);

// /**
// FUNCTION   :: UmtsMacFinalize
// LAYER      :: UMTS LAYER2 MAC Sublayer
// PURPOSE    :: Print stats and clear protocol variables.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : Interface index
// RETURN     :: void : NULL
// **/
void UmtsMacFinalize(Node* node, UInt32 interfaceIndex);

// /**
// FUNCTION   :: UmtsMacLayer
// LAYER      :: UMTS LAYER2 MAC sublayer
// PURPOSE    :: Handle timers and layer messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : Interface index
// + msg              : Message*          : Message for node to interpret.
// RETURN     :: void : NULL
// **/
void UmtsMacLayer(Node* node, UInt32 interfaceIndex, Message* msg);
#endif //CELLUAR_UMTS_LAYER2_MAC_H
