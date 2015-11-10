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

#ifndef _LAYER3_LTE_H_
#define _LAYER3_LTE_H_

#include <map>

#include "lte_common.h"
#include "layer2_lte_mac.h"
#include "layer3_lte_measurement.h"
#include "layer2_lte_rlc.h"
#include "layer2_lte_pdcp.h"
#include "lte_rrc_config.h"

//--------------------------------------------------------------------------
// Define
//--------------------------------------------------------------------------
#define LTE_DEFAULT_BEARER_ID (0)

#define RRC_LTE_DEFAULT_WAIT_RRC_CONNECTED_TIME (10*MILLI_SECOND)
#define RRC_LTE_DEFAULT_WAIT_RRC_CONNECTED_RECONF_TIME (10*MILLI_SECOND)
#define RRC_LTE_DEFAULT_RELOC_PREP_TIME                  ( 200*MILLI_SECOND)
#define RRC_LTE_DEFAULT_X2_RELOC_OVERALL_TIME            (1000*MILLI_SECOND)
#define RRC_LTE_DEFAULT_X2_WAIT_SN_STATUS_TRANSFER_TIME  ( 500*MILLI_SECOND)
#define RRC_LTE_DEFAULT_WAIT_ATTCH_UE_BY_HO_TIME         ( 500*MILLI_SECOND)
#define RRC_LTE_DEFAULT_X2_WAIT_END_MARKER_TIME          ( 200*MILLI_SECOND)
#define RRC_LTE_DEFAULT_S1_WAIT_PATH_SWITCH_REQ_ACK_TIME ( 200*MILLI_SECOND)
#define RRC_LTE_DEFAULT_MEAS_FILTER_COEFFICIENT (40.0)
#define RRC_LTE_DEFAULT_HO_IGNORED_TIME (0*SECOND)

#define RRC_LTE_STRING_WAIT_RRC_CONNECTED_TIME \
    "RRC-LTE-WAIT-RRC-CONNECTED-TIME"
#define RRC_LTE_STRING_WAIT_RRC_CONNECTED_RECONF_TIME \
    "RRC-LTE-WAIT-RRC-CONNECTED-RECONF-TIME"
#define RRC_LTE_STRING_MEAS_FILTER_COEFFICIENT \
    "RRC-LTE-MEAS-FILTER-COEFFICIENT"
#define RRC_LTE_STRING_IGNORE_HO_DECISION_TIME \
    "RRC-LTE-IGNORE-HO-DECISION-TIME"

#ifdef LTE_LIB_USE_POWER_TIMER
#define RRC_LTE_STRING_NUM_POWER_ON "RRC-LTE-NUM-POWER-ON"
#define RRC_LTE_STRING_NUM_POWER_OFF "RRC-LTE-NUM-POWER-OFF"
#define RRC_LTE_STRING_POWER_ON_TIME "RRC-LTE-POWER-ON-TIME"
#define RRC_LTE_STRING_POWER_OFF_TIME "RRC-LTE-POWER-OFF-TIME"
#endif

#define RRC_LTE_MAX_MEAS_FILTER_COEFFICIENT (100.0)

#define LTE_LIB_STATION_TYPE_GUARD(node, interfaceIndex, type, typeString) \
    ERROR_Assert(LteLayer2GetStationType(node, interfaceIndex) == type, \
        "This function should be called from only "typeString".")

//--------------------------------------------------------------------------
// Enumulation
//--------------------------------------------------------------------------
typedef enum {
    LAYER3_LTE_POWER_OFF,
    LAYER3_LTE_POWER_ON,
    LAYER3_LTE_RRC_IDLE,
    LAYER3_LTE_RRC_CONNECTED,
    LAYER3_LTE_STATUS_NUM
} Layer3LteState;

// -------------------------------------------------------------------------
// Basic data structs such as parameters, statistics, protocol data struc
//--------------------------------------------------------------------------

// /**
// STRUCT      :: LteLayer3StatData
// DESCRIPTION :: Statistics Data
// **/
typedef struct {
    int numRrcConnectionEstablishment;
    int averageRetryRrcConnectionEstablishment;
    clocktype averageTimeOfRrcConnectionEstablishment;
} LteLayer3StatData;

// /**
// STRUCT      :: LteLayer3EstablishmentStat
// DESCRIPTION :: Statistics Data for establishment
// **/
typedef struct {
    clocktype lastStartTime;
    clocktype numPowerOn;
    clocktype numPowerOff;
} LteLayer3EstablishmentStat;

typedef struct struct_rrc_connection_reconfiguration{
    LteRrcConfig rrcConfig;
    struct_rrc_connection_reconfiguration()
    {}
    struct_rrc_connection_reconfiguration(
        const struct_rrc_connection_reconfiguration& other)
    {
        rrcConfig = other.rrcConfig;    // just memberwise copy
    }
} RrcConnectionReconfiguration;

typedef enum {
    LAYER3_LTE_CONNECTION_WAITING,
    LAYER3_LTE_CONNECTION_CONNECTED,
    LAYER3_LTE_CONNECTION_HANDOVER,
    LAYER3_LTE_CONNECTION_STATUS_NUM
} Layer3LteConnectionState;

typedef struct struct_lte_radio_bearer
{
    LtePdcpEntity pdcpEntity;
    LteRlcEntity rlcEntity;

} LteRadioBearer;

typedef std::map < int, LteRadioBearer > MapLteRadioBearer;
typedef std::pair < int, LteRadioBearer > PairLteRadioBearer;

// /**
// STRUCT      :: LteConnectedInfo
// DESCRIPTION :: Connected Information
// **/
typedef struct struct_connected_info
{
    LteBsrInfo bsrInfo; // using only eNB
    MapLteRadioBearer radioBearers; // Radio Bearers
    clocktype connectedTime; // connectedTime
    BOOL isSchedulable;
} LteConnectedInfo;

typedef std::map < LteRnti, LteConnectedInfo > MapLteConnectedInfo;
typedef std::pair < LteRnti, LteConnectedInfo > PairLteConnectedInfo;

typedef struct struct_connection_info_lte
{
    Layer3LteConnectionState state;
    LteConnectedInfo connectedInfo;
    LteMapMessage mapLayer3Timer;

    HandoverParticipator hoParticipator;    // used ony for handover
#ifdef LTE_LIB_HO_VALIDATION
    UInt64 tempRecvByte;
#endif

    struct_connection_info_lte(Layer3LteConnectionState _state)
        : state(_state)
#ifdef LTE_LIB_HO_VALIDATION
        , tempRecvByte(0)
#endif
    {}
} LteConnectionInfo;

typedef std::map < LteRnti, LteConnectionInfo > MapLteConnectionInfo;
typedef std::pair < LteRnti, LteConnectionInfo > PairLteConnectionInfo;



typedef struct struct_rrc_connection_reconfiguration_including_mobCtrlInfo{
    HandoverParticipator hoParticipator;
    RrcConnectionReconfiguration reconf;
} RrcConnReconfInclMoblityControlInfomation;

typedef std::map<LteRnti, RrcConnReconfInclMoblityControlInfomation>
    RrcConnReconfInclMoblityControlInfomationMap;

typedef struct
    struct_rrc_connection_reconfiguration_including_mobCtrlInfo_container{
    int num;
    RrcConnReconfInclMoblityControlInfomation reconfList[1];
} RrcConnReconfInclMoblityControlInfomationContainer;

// /**
// STRUCT      :: Layer3DataLte
// DESCRIPTION :: Data structure of Lte model
// **/
typedef struct struct_layer3_lte_str
{
    Layer3LteState layer3LteState;

    MapLteConnectionInfo connectionInfoMap;

    LteLayer3StatData statData; // for statistics
    LteLayer3EstablishmentStat establishmentStatData; // for statistics
    clocktype waitRrcConnectedTime; // RRC-LTE-WAIT-RRC-CONNECTED-TIME
    clocktype waitRrcConnectedReconfTime; // RRC-LTE-WAIT-RRC-CONNECTED-RECONF-TIME
    clocktype ignoreHoDecisionTime; // RRC-LTE-IGNORE-HO-DECISION-TIME

    VarMeasConfig varMeasConfig;                        // measurement configuration
    MeasEventConditionTable measEventConditionTable;    // to manage event condition
    VarMeasReportList varMeasReportList;                // report information
    MapMeasPeriodicalTimer mapMeasPeriodicalTimer;      // map of MeasId and periodical timer

} Layer3DataLte;

//--------------------------------------------------------------------------
//  Utility functions
//--------------------------------------------------------------------------
// /**
// FUNCTION   :: Layer3LteGetConnectionInfo
// LAYER      :: RRC
// PURPOSE    :: Get Connection Info
// PARAMETERS ::
// + node           : Node*          : Pointer to node.
// + interfaceIndex : int            : Interface Index
// + rnti           : const LteRnti& : RNTI
// RETURN     :: LteConnectionInfo : Connection Info
// **/
LteConnectionInfo* Layer3LteGetConnectionInfo(
    Node* node, int interfaceIndex, const LteRnti& rnti);

// /**
// FUNCTION   :: Layer3LteGetConnectionList
// LAYER      :: RRC
// PURPOSE    :: Get connection List
// PARAMETERS ::
// + node           : Node*          : Pointer to node.
// + interfaceIndex : int            : Interface Index
// + store          : ListLteRnti*   : result
// + state       : Layer3LteConnectionState : state
// RETURN     :: void : NULL
// **/
void Layer3LteGetConnectionList(
    Node* node, int interfaceIndex, ListLteRnti* store,
    Layer3LteConnectionState state);

// /**
// FUNCTION   :: Layer3LteGetSchedulableListSortedByConnectedTime
// LAYER      :: RRC
// PURPOSE    :: Get connected List sorted by connected time
// PARAMETERS ::
// + node           : Node*          : Pointer to node.
// + interfaceIndex : int            : Interface Index
// + store          : ListLteRnti*   : result
// RETURN     :: void : NULL
// **/
void Layer3LteGetSchedulableListSortedByConnectedTime(
    Node* node, int interfaceIndex, ListLteRnti* store);

// /**
// FUNCTION   :: Layer3LteGetConnectionState
// LAYER      :: RRC
// PURPOSE    :: get connection state
// PARAMETERS ::
// + node           : Node*          : Pointer to node.
// + interfaceIndex : int            : Interface Index
// + rnti           : const LteRnti& : RNTI
// RETURN     :: Layer3LteConnectionState : state
// **/
Layer3LteConnectionState Layer3LteGetConnectionState(
    Node* node, int interfaceIndex, const LteRnti& rnti);

// /**
// FUNCTION   :: Layer3LteSetConnectionState
// LAYER      :: RRC
// PURPOSE    :: set connection state
// PARAMETERS ::
// + node           : Node*          : Pointer to node.
// + interfaceIndex : int            : Interface Index
// + rnti           : const LteRnti& : RNTI
// RETURN     :: void : NULL
// **/
void Layer3LteSetConnectionState(
    Node* node, int interfaceIndex, const LteRnti& rnti,
    Layer3LteConnectionState state);

// /**
// FUNCTION   :: Layer3LteGetHandoverParticipator
// LAYER      :: RRC
// PURPOSE    :: get participators of H.O.
// PARAMETERS ::
// + node           : Node*          : Pointer to node.
// + interfaceIndex : int            : Interface Index
// + rnti           : const LteRnti& : RNTI
// RETURN     :: HandoverParticipator : participators of H.O.
// **/
HandoverParticipator Layer3LteGetHandoverParticipator(
    Node* node, int interfaceIndex, const LteRnti& rnti);

// /**
// FUNCTION   :: Layer3LteSetHandoverParticipator
// LAYER      :: RRC
// PURPOSE    :: set participators of H.O.
// PARAMETERS ::
// + node           : Node*          : Pointer to node.
// + interfaceIndex : int            : Interface Index
// + rnti           : const LteRnti& : RNTI
// + hoParticipator   : const HandoverParticipator&: participators of H.O.
// RETURN     :: void : NULL
// **/
void Layer3LteSetHandoverParticipator(
    Node* node, int interfaceIndex, const LteRnti& rnti,
    const HandoverParticipator& hoParticipator);

// /**
// FUNCTION   :: Layer3LteChangeConnectionState
// LAYER      :: RRC
// PURPOSE    :: change connection state
// PARAMETERS ::
// + node           : Node*          : Pointer to node.
// + interfaceIndex : int            : Interface Index
// + rnti           : const LteRnti& : RNTI
// + newState       : Layer3LteConnectionState : new state
// RETURN     :: void : NULL
// **/
void Layer3LteChangeConnectionState(
    Node* node, int interfaceIndex, const LteRnti& rnti,
    Layer3LteConnectionState newState,
    const HandoverParticipator& hoParticipator = 
    HandoverParticipator());



// /**
// FUNCTION   :: Layer3LteGetHandoverInfo
// LAYER      :: RRC
// PURPOSE    :: get handover info
// PARAMETERS ::
// + node           : Node*          : Pointer to node.
// + interfaceIndex : int            : Interface Index
// + rnti           : const LteRnti& : RNTI
// RETURN     :: LteConnectionInfo* : info
// **/
LteConnectionInfo* Layer3LteGetHandoverInfo(
    Node* node, int interfaceIndex, const LteRnti& rnti = LTE_INVALID_RNTI);



// /**
// FUNCTION   :: Layer3LteGetConnectedEnb
// LAYER      :: RRC
// PURPOSE    :: Get Connected eNB
// PARAMETERS ::
// + node           : Node*          : Pointer to node.
// + interfaceIndex : int            : Interface Index
// RETURN     :: LteRnti : Connected eNB
// **/
const LteRnti Layer3LteGetConnectedEnb(Node* node, int interfaceIndex);

// /**
// FUNCTION   :: Layer3LteGetWaitingEnb
// LAYER      :: RRC
// PURPOSE    :: Get waiting eNB
// PARAMETERS ::
// + node           : Node*          : Pointer to node.
// + interfaceIndex : int            : Interface Index
// RETURN     :: LteRnti : waiting eNB
// **/
const LteRnti Layer3LteGetWaitingEnb(Node* node, int interfaceIndex);

// /**
// FUNCTION   :: Layer3LteGetRandomAccessingEnb
// LAYER      :: RRC
// PURPOSE    :: Get waiting eNB
// PARAMETERS ::
// + node           : Node*          : Pointer to node.
// + interfaceIndex : int            : Interface Index
// RETURN     :: LteRnti : waiting eNB
// **/
const LteRnti Layer3LteGetRandomAccessingEnb(Node* node, int interfaceIndex);

// /**
// FUNCTION   :: Layer3LteGetTimerForRrc
// LAYER      :: RRC
// PURPOSE    :: get timer message
// PARAMETERS ::
// + node            : Node*     : Pointer to node.
// + interfaceIndex  : int       : Interface index
// + rnti            : const LteRnti&    : RNTI
// + eventType       : int       : eventType in Message structure
// RETURN     :: Message* : timer message
// **/
Message* Layer3LteGetTimerForRrc(
    Node* node, int interfaceIndex, const LteRnti& rnti, int eventType);

// /**
// FUNCTION   :: Layer3LteAddTimerForRrc
// LAYER      :: RRC
// PURPOSE    :: add timer message
// PARAMETERS ::
// + node            : Node*     : Pointer to node.
// + interfaceIndex  : int       : Interface index
// + rnti            : const LteRnti&    : RNTI
// + eventType       : int       : eventType in Message structure
// + delay           : clocktype : delay before timer expired
// + timerMsg        : Message*  : timer added
// RETURN     :: void : NULL
// **/
void Layer3LteAddTimerForRrc(
    Node* node, int interfaceIndex, const LteRnti& rnti, int eventType,
    Message* timerMsg);

// /**
// FUNCTION   :: Layer3LteDeleteTimerForRrc
// LAYER      :: RRC
// PURPOSE    :: delete timer message
// PARAMETERS ::
// + node            : Node*     : Pointer to node.
// + interfaceIndex  : int       : Interface index
// + rnti            : const LteRnti&    : RNTI
// + eventType       : int       : eventType in Message structure
// RETURN     :: void : NULL
// **/
void Layer3LteDeleteTimerForRrc(
    Node* node, int interfaceIndex, const LteRnti& rnti, int eventType);

// /**
// FUNCTION   :: Layer3LteSetTimerForRrc
// LAYER      :: RRC
// PURPOSE    :: Set timer message
// PARAMETERS ::
// + node            : Node*     : Pointer to node.
// + interfaceIndex  : int       : Interface index
// + rnti            : const LteRnti&    : RNTI
// + eventType       : int       : eventType in Message structure
// + delay           : clocktype : delay before timer expired
// RETURN     :: void : NULL
// **/
void Layer3LteSetTimerForRrc(
    Node* node, int interfaceIndex, const LteRnti& rnti, int eventType,
    clocktype delay);

// /**
// FUNCTION   :: Layer3LteCancelTimerForRrc
// LAYER      :: RRC
// PURPOSE    :: Cancel timer
// PARAMETERS ::
// + node            : Node*     : Pointer to node.
// + interfaceIndex  : int       : Interface index
// + rnti            : const LteRnti&    : RNTI
// + eventType       : int       : eventType in Message structure
// RETURN     :: void : NULL
// **/
void Layer3LteCancelTimerForRrc(
    Node* node, int interfaceIndex, const LteRnti& rnti, int eventType);

// /**
// FUNCTION   :: Layer3LteCancelAllTimerForRrc
// LAYER      :: RRC
// PURPOSE    :: Cancel all the timers
// PARAMETERS ::
// + node            : Node*     : Pointer to node.
// + interfaceIndex  : int       : Interface index
// + rnti            : const LteRnti&    : RNTI
// RETURN     :: void : NULL
// **/
void Layer3LteCancelAllTimerForRrc(
    Node* node, int interfaceIndex, const LteRnti& rnti);

// /**
// FUNCTION   :: Layer3LteCheckTimerForRrc
// LAYER      :: RRC
// PURPOSE    :: Check timer
// PARAMETERS ::
// + node            : Node*     : Pointer to node.
// + interfaceIndex  : int       : Interface index
// + rnti            : const LteRnti&    : RNTI
// + eventType       : int       : eventType in Message structure
// RETURN     :: TRUE:Running FALSE:Not Running
// **/
BOOL Layer3LteCheckTimerForRrc(
    Node* node, int interfaceIndex, const LteRnti& rnti, int eventType);

////////////////////////////////////////////////////////////////////////////
// eNB/UE - API for Common
////////////////////////////////////////////////////////////////////////////
// /**
// FUNCTION   :: Layer3LteProcessEvent
// LAYER      :: RRC
// PURPOSE    :: Process Event.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + msg              : Message           : Event message
// RETURN     :: void : NULL
// **/
void Layer3LteProcessEvent(Node* node, UInt32 interfaceIndex, Message* msg);

// /**
// FUNCTION   :: Layer3LteAddConnectionInfo
// LAYER      :: RRC
// PURPOSE    :: Add Connected RNTI to List & Create Radio Bearer
// PARAMETERS ::
// + node           : Node*          : Pointer to node.
// + interfaceIndex : int            : Interface Index
// + rnti           : const LteRnti& : RNTI
// + state          : Layer3LteConnectionState : initial state
// RETURN     :: BOOL : TRUE:exist FALSE:not exist
// **/
void Layer3LteAddConnectionInfo(
    Node* node, int interfaceIndex, const LteRnti& rnti,
    Layer3LteConnectionState state);

// /**
// FUNCTION   :: Layer3LteRemoveConnectionInfo
// LAYER      :: RRC
// PURPOSE    :: Remove Connected info
// PARAMETERS ::
// + node           : Node*          : Pointer to node.
// + interfaceIndex : int            : Interface Index
// + rnti           : const LteRnti& : RNTI
// RETURN     :: BOOL : TRUE:exist FALSE:not exist
// **/
BOOL Layer3LteRemoveConnectionInfo(
    Node* node, int interfaceIndex, const LteRnti& rnti);

// /**
// FUNCTION   :: Layer3LtePowerOn
// LAYER      :: Layer3
// PURPOSE    :: Power ON
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// RETURN     :: void : NULL
// **/
void Layer3LtePowerOn(Node* node, int interfaceIndex);

// /**
// FUNCTION   :: Layer3LtePowerOff
// LAYER      :: Layer3
// PURPOSE    :: Power OFF
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// RETURN     :: void : NULL
// **/
void Layer3LtePowerOff(Node* node, int interfaceIndex);



// /**
// FUNCTION   :: Layer3LteRestart
// LAYER      :: RRC
// PURPOSE    :: Restart to search cells for re-attach
// PARAMETERS ::
// + node           : Node*             : Pointer to node.
// + interfaceIndex : int               : Interface Index
// RETURN     :: void : NULL
// **/
void 
Layer3LteRestart(Node* node, int interfaceIndex);



// /**
// FUNCTION   :: Layer3LteAddRoute
// LAYER      :: Layer3
// PURPOSE    :: Add Route
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + oppositeRnti     : const LteRnti&    : Oppsite RNTI
// RETURN     :: void : NULL
// NOTE       :: this function is not supported at Phase 1
// **/
void Layer3LteAddRoute(
    Node* node, int interfaceIndex, const LteRnti& oppositeRnti);



// /**
// FUNCTION   :: Layer3LteDeleteRoute
// LAYER      :: Layer3
// PURPOSE    :: Delete Route
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + oppositeRnti     : const LteRnti&    : Oppsite RNTI
// RETURN     :: void : NULL
// **/
void Layer3LteDeleteRoute(
    Node* node, int interfaceIndex, const LteRnti& oppositeRnti);



////////////////////////////////////////////////////////////////////////////
// eNB - API for Common
////////////////////////////////////////////////////////////////////////////
// /**
// FUNCTION   :: layer3LteInitialize
// LAYER      :: RRC
// PURPOSE    :: Init RRC
// PARAMETERS ::
// + node           : Node*             : Pointer to node.
// + interfaceIndex : int               : Interface Index
// + nodeInput      : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
void Layer3LteInitialize(
    Node* node, int interfaceIndex, const NodeInput* nodeInput);

// /**
// FUNCTION   :: layer3LteFinalize
// LAYER      :: RRC
// PURPOSE    :: Finalize RRC
// PARAMETERS ::
// + node           : Node*             : Pointer to node.
// + interfaceIndex : int               : Interface Index
// RETURN     :: void : NULL
// **/
void Layer3LteFinalize(
    Node* node, int interfaceIndex);

// /**
// FUNCTION   :: Layer3LteNotifyBsrFromMac
// LAYER      :: RRC
// PURPOSE    :: SAP for BSR Notification from eNB's MAC LAYER
// PARAMETERS ::
// + node           : Node*      : Pointer to node.
// + interfaceIndex : int        : Interface Index
// + bsrInfo        : LteBsrInfo : BSR Info Structure
// RETURN     :: void : NULL
// **/
void Layer3LteNotifyBsrFromMac(
    Node* node, int interfaceIndex, const LteBsrInfo& bsrInfo);

////////////////////////////////////////////////////////////////////////////
// UE - API for Common
////////////////////////////////////////////////////////////////////////////
// /**
// FUNCTION   :: Layer3LteProcessRrcConnectedTimerExpired
// LAYER      :: RRC
// PURPOSE    :: Process RRC_CONNECTED Timer expire event.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + msg              : Message           : Event message
// RETURN     :: void : NULL
// **/
void Layer3LteProcessRrcConnectedTimerExpired(
    Node* node, UInt32 interfaceIndex, Message* msg);



// /**
// FUNCTION   :: Layer3LteProcessRrcConnectedReconfTimerExpired
// LAYER      :: RRC
// PURPOSE    :: Process RRC_CONNECTED_RECONF Timer expire event.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + msg              : Message*          : Event message
// + connectedInfo    : LteConnectedInfo* : connectedInfo used before H.O.
// RETURN     :: void : NULL
// **/
void Layer3LteProcessRrcConnectedReconfTimerExpired(
    Node* node, UInt32 interfaceIndex, Message* msg);

// /**
// FUNCTION   :: Layer3LteDetachUE
// LAYER      :: RRC
// PURPOSE    :: Detach UE
// PARAMETERS ::
// + node           : Node*             : Pointer to node.
// + interfaceIndex : int               : Interface Index
// + ueRnti     : const LteRnti& oppositeRnti : RNTI of detached UE
// RETURN     :: void : NULL
// **/
void Layer3LteDetachUE(
    Node* node, int interfaceIndex, const LteRnti& oppositeRnti);



////////////////////////////////////////////////////////////////////////////
// eNB/UE - API for PHY
////////////////////////////////////////////////////////////////////////////
// /**
// FUNCTION   :: Layer3LteNotifyLostDetection
// LAYER      :: RRC
// PURPOSE    :: Notify lost detection
// PARAMETERS ::
// + node           : Node*             : Pointer to node.
// + interfaceIndex : int               : Interface Index
// + ueRnti     : LteRnti oppositeRnti : RNTI lost
// RETURN     :: void : NULL
// **/
void Layer3LteNotifyLostDetection(
    Node* node, int interfaceIndex, LteRnti oppositeRnti);



// /**
// FUNCTION   :: Layer3LteNotifyFailureRAToTargetEnbOnHandover
// LAYER      :: RRC
// PURPOSE    :: Notify lost detection
// PARAMETERS ::
// + node           : Node*             : Pointer to node.
// + interfaceIndex : int               : Interface Index
// RETURN     :: void : NULL
// **/
void Layer3LteNotifyFailureRAToTargetEnbOnHandover(
    Node* node, int interfaceIndex);



// /**
// FUNCTION   :: Layer3LteSetupMeasConfig
// LAYER      :: Layer3
// PURPOSE    :: Set Up Measurement Configuration
// PARAMETERS ::
// + node           : Node*             : Pointer to node.
// + interfaceIndex : int               : Interface Index
// + nodeInput      : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
void Layer3LteSetupMeasConfig(
    Node* node, int interfaceIndex, const NodeInput* nodeInput);

// /**
// FUNCTION   :: Layer3LteSetupMeasEventConditionTable
// LAYER      :: RRC
// PURPOSE    :: Setup Event Condition Observing Information
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// RETURN     :: void : NULL
// **/
void Layer3LteSetupMeasEventConditionTable(Node *node,
                    UInt32 interfaceIndex);
// /**
// FUNCTION   :: Layer3LteInitMeasurement
// LAYER      :: RRC
// PURPOSE    :: initialize measurement
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// RETURN     :: void : NULL
// **/
void Layer3LteInitMeasurement(
    Node* node, UInt32 interfaceIndex);

// /**
// FUNCTION   :: Layer3LteIFHPNotifyL3FilteringUpdated
// LAYER      :: RRC
// PURPOSE    :: IF for PHY to notify about measurement result updated
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// RETURN     :: void : NULL
// **/
void Layer3LteIFHPNotifyL3FilteringUpdated(
                    Node *node,
                    UInt32 interfaceIndex);

// /**
// FUNCTION   :: Layer3LteIFHPNotifyMeasurementReportReceived
// LAYER      :: RRC
// PURPOSE    :: IF for PHY to notify Measurement Repot Received to RRC
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + ueRnti           : const LteRnti&    : the source of the report
// + measurementReport: std::list<MeasurementReport>*: report
// RETURN     :: void : NULL
// **/
void Layer3LteIFHPNotifyMeasurementReportReceived(
                    Node *node,
                    UInt32 interfaceIndex,
                    const LteRnti& ueRnti,
                    std::list<MeasurementReport>* measurementReport);
// /**
// FUNCTION   :: Layer3LteHandOverDecision
// LAYER      :: RRC
// PURPOSE    :: handover decision
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + ueRnti           : const LteRnti&    : the source of the report
// + measurementReport: std::list<MeasurementReport>*: report
// + targetRnti       : LteRnti*          : target eNB (if INVALID_RNTI
//                                          is set, doesn't hand over)
// RETURN     :: void : NULL
// **/
void Layer3LteHandOverDecision(
                    Node *node,
                    UInt32 interfaceIndex,
                    const LteRnti& ueRnti,
                    std::list<MeasurementReport>* measurementReport,
                    LteRnti* targetRnti);
// /**
// FUNCTION   :: Layer3LteAdmissionControl
// LAYER      :: RRC
// PURPOSE    :: admission control
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + hoParticipator   : const HandoverParticipator&: participators of H.O.
// RETURN     :: BOOL : result
// **/
BOOL Layer3LteAdmissionControl(
                    Node *node,
                    UInt32 interfaceIndex,
                    const HandoverParticipator& hoParticipator);
// /**
// FUNCTION   :: Layer3LteReceiveHoReq
// LAYER      :: RRC
// PURPOSE    :: process after receive handover request
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + hoParticipator   : const HandoverParticipator&: participators of H.O.
// RETURN     :: void : NULL
// **/
void Layer3LteReceiveHoReq(
                    Node *node,
                    UInt32 interfaceIndex,
                    const HandoverParticipator& hoParticipator);
// /**
// FUNCTION   :: Layer3LteReceiveHoReqAck
// LAYER      :: RRC
// PURPOSE    :: process after receive handover request ack
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + hoParticipator   : const HandoverParticipator&: participators of H.O.
// + reconf   : const RrcConnectionReconfiguration& reconf : reconfiguration
// RETURN     :: void : NULL
// **/
void Layer3LteReceiveHoReqAck(
                    Node *node,
                    UInt32 interfaceIndex,
                    const HandoverParticipator& hoParticipator,
                    const RrcConnectionReconfiguration& reconf);

// /**
// FUNCTION   :: Layer3LteSendSnStatusTransfer
// LAYER      :: RRC
// PURPOSE    :: send SnStatusTransfer
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + hoParticipator   : const HandoverParticipator&: participators of H.O.
// RETURN     :: void : NULL
// **/
void Layer3LteSendSnStatusTransfer(
    Node *node,
    UInt32 interfaceIndex,
    const HandoverParticipator& hoParticipator);

// /**
// FUNCTION   :: Layer3LteReceiveSnStatusTransfer
// LAYER      :: RRC
// PURPOSE    :: process after receive SnStatusTransfer
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + hoParticipator   : const HandoverParticipator&: participators of H.O.
// + snStatusTransferItem : std::map<int, PdcpLteSnStatusTransferItem>& : 
// RETURN     :: void : NULL
// **/
void Layer3LteReceiveSnStatusTransfer(
    Node *node,
    UInt32 interfaceIndex,
    const HandoverParticipator& hoParticipator,
    std::map<int, PdcpLteSnStatusTransferItem>& snStatusTransferItem);

// /**
// FUNCTION   :: Layer3LteSendRrcConnReconfInclMobilityControlInfomation
// LAYER      :: RRC
// PURPOSE    :: send reconfiguration to handovering UE
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + hoParticipator   : const HandoverParticipator&: participators of H.O.
// + reconf   : const RrcConnectionReconfiguration& reconf : reconfiguration
// RETURN     :: void : NULL
// **/
void Layer3LteSendRrcConnReconfInclMobilityControlInfomation(
                    Node *node,
                    UInt32 interfaceIndex,
                    const HandoverParticipator& hoParticipator,
                    const RrcConnectionReconfiguration& reconf);
// /**
// FUNCTION   :: Layer3LteNotifyRrcConnReconf
// LAYER      :: RRC
// PURPOSE    :: notify reconfiguration received
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + hoParticipator   : const HandoverParticipator&: participators of H.O.
// + reconf   : const RrcConnectionReconfiguration& reconf : reconfiguration
// RETURN     :: void : NULL
// **/
void Layer3LteNotifyRrcConnReconf(
                    Node *node,
                    UInt32 interfaceIndex,
                    const HandoverParticipator& hoParticipator,
                    const RrcConnectionReconfiguration& reconf);
// /**
// FUNCTION   :: Layer3LtePrepareForHandoverExecution
// LAYER      :: RRC
// PURPOSE    :: clear information for handover execution
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + hoParticipator   : const HandoverParticipator&: participators of H.O.
// RETURN     :: void : NULL
// **/
void Layer3LtePrepareForHandoverExecution(Node* node, int interfaceIndex,
                    const HandoverParticipator& hoParticipator);

// /**
// FUNCTION   :: Layer3LtePrepareDataForwarding
// LAYER      :: RRC
// PURPOSE    :: clear information for handover execution
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + hoParticipator   : const HandoverParticipator&: participators of H.O.
// RETURN     :: void : NULL
// **/
void Layer3LtePrepareDataForwarding(
    Node* node, int interfaceIndex,
    const HandoverParticipator& hoParticipator);

// /**
// FUNCTION   :: Layer3LteStartDataForwarding
// LAYER      :: RRC
// PURPOSE    :: start data forwarding process
// PARAMETERS ::
// + node           : Node*          : Pointer to node.
// + interfaceIndex : int            : Interface Index
// + hoParticipator   : const HandoverParticipator&: participators of H.O.
// RETURN     :: void : NULL
// **/
void Layer3LteStartDataForwarding(Node* node, int interfaceIndex,
                              const HandoverParticipator& hoParticipator);

// /**
// FUNCTION   :: Layer3LteSendForwardingDataList
// LAYER      :: RRC
// PURPOSE    :: send forwarding data list
// PARAMETERS ::
// + node           : Node*          : Pointer to node.
// + interfaceIndex : int            : Interface Index
// + rnti           : const LteRnti& : UE's RNTI
// + bearerId       : int            : bearer ID of the message list
// + forwardingMsg  : std::list<Message*> : message list
// RETURN     :: void : NULL
// **/
void Layer3LteSendForwardingDataList(Node* node, int interfaceIndex,
    const LteRnti& ueRnti, int bearerId, std::list<Message*>* forwardingMsg);

// /**
// FUNCTION   :: Layer3LteReceiveDataForwarding
// LAYER      :: RRC
// PURPOSE    :: process after receive DataForwardin
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + hoParticipator   : const HandoverParticipator&: participators of H.O.
// + forwardingMsg  : Message* : message
// RETURN     :: void : NULL
// **/
void Layer3LteReceiveDataForwarding(
    Node* node,
    int interfaceIndex,
    const HandoverParticipator& hoParticipator,
    int bearerId,
    Message* forwardingMsg);

// /**
// FUNCTION   :: Layer3LteReceivePathSwitchRequestAck
// LAYER      :: RRC
// PURPOSE    :: receive path switch request ack
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + hoParticipator   : const HandoverParticipator&: participators of H.O.
// + result           : BOOL     : path switch result
// RETURN     :: void : NULL
// **/
void Layer3LteReceivePathSwitchRequestAck(
    Node* node,
    int interfaceIndex,
    const HandoverParticipator& hoParticipator,
    BOOL result);

// /**
// FUNCTION   :: Layer3LteReceiveEndMarker
// LAYER      :: RRC
// PURPOSE    :: receive end marker
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + hoParticipator   : const HandoverParticipator&: participators of H.O.
// RETURN     :: void : NULL
// **/
void Layer3LteReceiveEndMarker(
    Node* node,
    int interfaceIndex,
    const HandoverParticipator& hoParticipator);

// /**
// FUNCTION   :: Layer3LteReceiveUeContextRelease
// LAYER      :: RRC
// PURPOSE    :: receive ue context release
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + hoParticipator   : const HandoverParticipator&: participators of H.O.
// RETURN     :: void : NULL
// **/
void Layer3LteReceiveUeContextRelease(
    Node* node,
    int interfaceIndex,
    const HandoverParticipator& hoParticipator);

// /**
// FUNCTION   :: Layer3LteReleaseUeContext
// LAYER      :: RRC
// PURPOSE    :: release ue context
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + hoParticipator   : const HandoverParticipator&: participators of H.O.
// RETURN     :: void : NULL
// **/
void Layer3LteReleaseUeContext(
    Node* node,
    int interfaceIndex,
    const HandoverParticipator& hoParticipator);

// /**
// FUNCTION   :: Layer3LteReceiveHoPreparationFailure
// LAYER      :: RRC
// PURPOSE    :: receive ho preparation failure
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + hoParticipator   : const HandoverParticipator&: participators of H.O.
// RETURN     :: void : NULL
// **/
void Layer3LteReceiveHoPreparationFailure(
    Node* node,
    int interfaceIndex,
    const HandoverParticipator& hoParticipator);



#endif // _LAYER3_LTE_H_

