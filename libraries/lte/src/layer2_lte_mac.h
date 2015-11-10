#ifndef _LAYER2_LTE_MAC_H_
#define _LAYER2_LTE_MAC_H_

#include "lte_common.h"
#include "parallel.h"
#ifdef LTE_LIB_LOG
#include "log_lte.h"
#endif

////////////////////////////////////////////////////////////////////////////
// Define
////////////////////////////////////////////////////////////////////////////
#define MAC_LTE_RA_PREAMBLE_INDEX_MAX (64)

#define MAC_LTE_DEFAULT_RA_DELTA_PREAMBLE (0)
#define MAC_LTE_DEFAULT_RA_PRACH_MASK_INDEX (0)

#define MAC_LTE_RRELCIDFL_WITH_7BIT_SUBHEADER_SIZE (2)
#define MAC_LTE_RRELCIDFL_WITH_15BIT_SUBHEADER_SIZE (3)
#define MAC_LTE_RRELCID_SUBHEADER_SIZE (1)
#define MAC_LTE_DEFAULT_F_FIELD (1)
#define MAC_LTE_DEFAULT_LCID_FIELD (0)
#define MAC_LTE_DEFAULT_E_FIELD (0)

// default parameter
#define MAC_LTE_DEFAULT_RA_BACKOFF_TIME (10*MILLI_SECOND)
#define MAC_LTE_DEFAULT_RA_PREAMBLE_INITIAL_RECEIVED_TARGET_POWER (-90.0)
#define MAC_LTE_DEFAULT_RA_POWER_RAMPING_STEP (2.0)
#define MAC_LTE_DEFAULT_RA_PREAMBLE_TRANS_MAX (4)
#define MAC_LTE_DEFAULT_RA_RESPONSE_WINDOW_SIZE (10)
#define MAC_LTE_DEFAULT_RA_PRACH_CONFIG_INDEX (14)
#define MAC_LTE_DEFAULT_PERIODIC_BSR_TTI (1)
#define MAC_LTE_DEFAULT_ENB_SCHEDULER_TYPE (LTE_SCH_ENB_RR)
#define MAC_LTE_DEFAULT_UE_SCHEDULER_TYPE (LTE_SCH_UE_SIMPLE)
#define MAC_LTE_DEFAULT_TRANSMISSION_MODE (1)

// parameter strings
#define MAC_LTE_STRING_RA_BACKOFF_TIME       "MAC-LTE-RA-BACKOFF-TIME"
#define MAC_LTE_STRING_RA_PREAMBLE_INITIAL_RECEIVED_TARGET_POWER \
    "MAC-LTE-RA-PREAMBLE-INITIAL-RECEIVED-TARGET-POWER"
#define MAC_LTE_STRING_RA_POWER_RAMPING_STEP "MAC-LTE-RA-POWER-RAMPING-STEP"
#define MAC_LTE_STRING_RA_PREAMBLE_TRANS_MAX "MAC-LTE-RA-PREAMBLE-TRANS-MAX"
#define MAC_LTE_STRING_RA_RESPONSE_WINDOW_SIZE \
    "MAC-LTE-RA-RESPONSE-WINDOW-SIZE"
#define MAC_LTE_STRING_RA_PRACH_CONFIG_INDEX  "MAC-LTE-RA-PRACH-CONFIG-INDEX"
#define MAC_LTE_STRING_NUM_SF_PER_TTI         "MAC-LTE-NUM-SF-PER-TTI"
#define MAC_LTE_STRING_PERIODIC_BSR_TTI       "MAC-LTE-PERIODIC-BSR-TTI"
#define MAC_LTE_STRING_ENB_SCHEDULER_TYPE     "MAC-LTE-ENB-SCHEDULER-TYPE"
#define MAC_LTE_STRING_UE_SCHEDULER_TYPE      "MAC-LTE-UE-SCHEDULER-TYPE"
#define MAC_LTE_STRING_TRANSMISSION_MODE      "MAC-LTE-TRANSMISSION-MODE"

////////////////////////////////////////////////////////////////////////////
// Enumlation
////////////////////////////////////////////////////////////////////////////
typedef enum
{
    LTE_SCH_UE_SIMPLE, // SIMPLE-SCHEDULER
    LTE_SCH_ENB_RR, // ROUND-ROBIN
    LTE_SCH_ENB_PF, // PROPORTIONAL-FAIRNESS
    LTE_SCH_NUM
} LTE_SCHEDULER_TYPE;

typedef enum {
    MAC_LTE_POWER_OFF,
    MAC_LTE_IDEL,
    MAC_LTE_RA_GRANT_WAITING,
    MAC_LTE_RA_BACKOFF_WAITING,
    MAC_LTE_DEFAULT_STATUS,
    MAC_LTE_STATUS_NUM
} MacLteState;
////////////////////////////////////////////////////////////////////////////
// Structure
////////////////////////////////////////////////////////////////////////////
// /**
// STRUCT      :: LteMacRrelcidflWith15bitSubHeader
// DESCRIPTION :: R/R/E/LCID/F/L sub-header with 7-bit L field
// NOTE        :: This and actual size are diferent.
// **/
typedef struct {
    UInt8 e;
    UInt8 lcid;
    UInt8 f;
    UInt8 l;
} LteMacRrelcidflWith7bitSubHeader;

// /**
// STRUCT      :: LteMacRrelcidflWith15bitSubHeader
// DESCRIPTION :: R/R/E/LCID/F/L sub-header with 15-bit L field
// NOTE        :: This and actual size are diferent.
// **/
typedef struct {
    UInt8 e;
    UInt8 lcid;
    UInt8 f;
    UInt16 l;
} LteMacRrelcidflWith15bitSubHeader;

// /**
// STRUCT      :: LteMacRrelcidSubHeader
// DESCRIPTION :: R/R/E/LCID sub-header
// NOTE        :: This and actual size are diferent.
// **/
typedef struct {
    UInt8 e;
    UInt8 lcid;
} LteMacRrelcidSubHeader;

typedef struct {
    // UE   Number of sending Random Access Preamble
    UInt32 numberOfSendingRaPreamble;
    // ENB  Number of receiving Random Access Preamble
    UInt32 numberOfRecievingRaPreamble;
    // ENB  Number of sending Random Access Grant
    UInt32 numberOfSendingRaGrant;
    // UE   Number of receiving Random Access Grant
    UInt32 numberOfRecievingRaGrant;
    // UE   Number of establishment
    UInt32 numberOfEstablishment;

    UInt32 numberOfSduFromUpperLayer;
    UInt32 numberOfPduToLowerLayer;
    UInt32 numberOfPduFromLowerLayer;
    UInt32 numberOfPduFromLowerLayerWithError;
    UInt32 numberOfSduToUpperLayer;
} LteMacStatData;

// /**
// STRUCT      :: LteMacData
// DESCRIPTION :: MAC Sublayer's data
// **/
typedef struct {

    // configurable parameter
    LTE_SCHEDULER_TYPE enbSchType;    //MAC-LTE-ENB-SCHEDULER-TYPE
    LTE_SCHEDULER_TYPE ueSchType;    //MAC-LTE-UE-SCHEDULER-TYPE

    // random seed
    RandomSeed seed;

    // State Variable
    UInt64 ttiNumber; // TTI Number
    int preambleTransmissionCounter; // PREAMBLE_TRANSMISSION_COUNTER

    // Stat data
    LteMacStatData statData;

    // State
    MacLteState macState;

    // Timer
    LteMapMessage mapMacTimer;

    // Last propagation delay
    clocktype lastPropDelay;

#ifdef LTE_LIB_LOG
#ifdef LTE_LIB_VALIDATION_LOG
    std::map < LteRnti, lte::LogLteAverager >* avgEstimatedSinrDl;
    std::map < LteRnti, lte::LogLteAverager >* avgEstimatedSinrUl;
    std::map < LteRnti, lte::LogLteAverager >* avgNumAllocRbDl;
    std::map < LteRnti, lte::LogLteAverager >* avgNumAllocRbUl;
#endif
#endif
#ifdef PARALLEL
    LookaheadHandle lookaheadHandle;
#endif

} LteMacData;

// /**
// STRUCT      :: LteBsrInfo
// DESCRIPTION :: Buffer Status Reporting Info
// **/
typedef struct {
    LteRnti ueRnti; // UE's RNTI
    int bufferSizeLevel; // [0-63]
    UInt32 bufferSizeByte; // Sendable size in TX and/or RE-TX Buffer in RLC
} LteBsrInfo;

// /**
// STRUCT      :: DlTtiInfo
// DESCRIPTION :: DL Information sending to each TTI.
// **/
typedef struct {
    UInt64 ttiNumber;
} DlTtiInfo;

// /**
// STRUCT      :: MacLteTxInfo
// DESCRIPTION :: Tx Information for MAC.
// **/
typedef struct {
    UInt32 numResourceBlocks;
} MacLteTxInfo;

////////////////////////////////////////////////////////////////////////////
// Function for MAC Layer
////////////////////////////////////////////////////////////////////////////
// /**
// FUNCTION   :: MacLteInit
// LAYER      :: MAC
// PURPOSE    :: Initialize LTE MAC Layer.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
void MacLteInit(Node* node,
                UInt32 interfaceIndex,
                const NodeInput* nodeInput);

// /**
// FUNCTION   :: MacLteFinalize
// LAYER      :: MAC
// PURPOSE    :: Print stats and clear protocol variables.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// RETURN     :: void : NULL
// **/
void MacLteFinalize(Node* node, UInt32 interfaceIndex);

// /**
// FUNCTION   :: MacLteProcessEvent
// LAYER      :: MAC
// PURPOSE    :: Process Event.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + msg              : Message           : Event message
// RETURN     :: void : NULL
// **/
void MacLteProcessEvent(Node* node, UInt32 interfaceIndex, Message* msg);

// /**
// FUNCTION   :: MacLteProcessTtiTimerExpired
// LAYER      :: MAC
// PURPOSE    :: Process TTI Timer expire event.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + msg              : Message           : Event message
// RETURN     :: void : NULL
// **/
void MacLteProcessTtiTimerExpired(
    Node* node, UInt32 interfaceIndex, Message* msg);

// /**
// FUNCTION   :: MacLteStartSchedulingAtENB
// LAYER      :: MAC
// PURPOSE    :: Start point for eNB scheduling.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + msg              : Message           : Event message
// RETURN     :: void : NULL
// **/
void MacLteStartSchedulingAtENB(
    Node* node, UInt32 interfaceIndex, Message* msg);

// /**
// FUNCTION   :: MacLteStartSchedulingAtUE
// LAYER      :: MAC
// PURPOSE    :: Start point for UE scheduling.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + msg              : Message           : Event message
// RETURN     :: void : NULL
// **/
void MacLteStartSchedulingAtUE(
    Node* node, UInt32 interfaceIndex, Message* msg);

// /**
// FUNCTION   :: MacLteAddDestinationInfo
// LAYER      :: MAC
// PURPOSE    :: Add Destination Info
// PARAMETERS ::
// + node               : IN : Node*    : Pointer to node.
// + interfaceIndex     : IN : Int32      : Interface index
// + msg                : IN : Message* : MAC PDU Message Structure
// + oppositeRnti       : IN : LteRnti  : Oposite RNTI
// RETURN     :: void : NULL
// **/
void MacLteAddDestinationInfo(Node* node,
                              Int32 interfaceIndex,
                              Message* msg,
                              const LteRnti& oppositeRnti);

// /**
// FUNCTION   :: MacLteGetBsrLevel
// LAYER      :: MAC
// PURPOSE    :: Get BSR Level from size[byte].
// PARAMETERS ::
// + size             : BSR size[byte].
// RETURN     :: int  : BSR Level
// **/
int MacLteGetBsrLevel(UInt32 size);

// /**
// FUNCTION   :: MacLteSetNextTtiTimer
// LAYER      :: MAC
// PURPOSE    :: Set next TTI timer
// PARAMETERS ::
// + node               : IN : Node*  : Pointer to node.
// + interfaceIndex     : IN : int    : Interface index
// RETURN     :: void : NULL
// **/
void MacLteSetNextTtiTimer(Node* node, int interfaceIndex);

// /**
// FUNCTION   :: MacLteSetTimerForMac
// LAYER      :: MAC
// PURPOSE    :: Set timer message
// PARAMETERS ::
// + node            : IN : Node*     : Pointer to node.
// + interfaceIndex  : IN : int       : Interface index
// + eventType       : IN : int       : eventType in Message structure
// + delay           : IN : clocktype : delay before timer expired
// RETURN     :: void : NULL
// **/
void MacLteSetTimerForMac(
    Node* node, int interfaceIndex, int eventType, clocktype delay);

// /**
// FUNCTION   :: MacLteGetTimerForMac
// LAYER      :: MAC
// PURPOSE    :: Get timer message
// PARAMETERS ::
// + node            : IN : Node*     : Pointer to node.
// + interfaceIndex  : IN : int       : Interface index
// + eventType       : IN : int       : eventType in Message structure
// RETURN     :: Message* : Timer Message
// **/
Message* MacLteGetTimerForMac(
    Node* node, int interfaceIndex, int eventType);

// /**
// FUNCTION   :: MacLteCancelTimerForMac
// LAYER      :: MAC
// PURPOSE    :: Cancel timer
// PARAMETERS ::
// + node            : IN : Node*     : Pointer to node.
// + interfaceIndex  : IN : int       : Interface index
// + eventType       : IN : int       : eventType in Message structure
// + timerMsg        : IN : Message*  : Pointer to timer message
// RETURN     :: void : NULL
// **/
void MacLteCancelTimerForMac(
    Node* node, int interfaceIndex, int eventType, Message* timerMsg);

// /**
// FUNCTION   :: MacLteSetState
// LAYER      :: MAC
// PURPOSE    :: Set state
// PARAMETERS ::
// + node               : IN : Node*       : Pointer to node.
// + interfaceIndex     : IN : int         : Interface index
// + state              : IN : MacLteState : State
// RETURN     :: void : NULL
// **/
void MacLteSetState(Node* node, int interfaceIndex, MacLteState state);

////////////////////////////////////////////////////////////////////////////
// eNB/UE - API for Common
////////////////////////////////////////////////////////////////////////////
// /**
// FUNCTION   :: MacLteGetTtiNumber
// LAYER      :: MAC
// PURPOSE    :: Get TTI Number
// PARAMETERS ::
// + node           : Node* : Pointer to node.
// + interfaceIndex : int   : Interface Index
// RETURN     :: UInt64 : TTI Number
// **/
UInt64 MacLteGetTtiNumber(Node* node, int interfaceIndex);

// /**
// FUNCTION   :: MacLteSetTtiNumber
// LAYER      :: MAC
// PURPOSE    :: Set TTI Number
// PARAMETERS ::
// + node           : Node* : Pointer to node.
// + interfaceIndex : int   : Interface Index
// + ttiNumber      : UInt64: TTI Number
// RETURN     :: void : NULL
// **/
void MacLteSetTtiNumber(Node* node, int interfaceIndex, UInt64 ttiNumber);

// /**
// FUNCTION   :: MacLteGetNumSubframePerTti
// LAYER      :: MAC
// PURPOSE    :: Get number of subframe per TTI
// PARAMETERS ::
// + node           : Node* : Pointer to node.
// + interfaceIndex : int   : Interface Index
// RETURN     :: int : Number of subframe per TTI
// **/
int MacLteGetNumSubframePerTti(
    Node* node, int interfaceIndex);

// /**
// FUNCTION   :: MacLteGetTtiLength
// LAYER      :: MAC
// PURPOSE    :: Get TTI length [nsec]
// PARAMETERS ::
// + node           : Node* : Pointer to node.
// + interfaceIndex : int   : Interface Index
// RETURN     :: clocktype : TTI length
// **/
clocktype MacLteGetTtiLength(
    Node* node, int interfaceIndex);

////////////////////////////////////////////////////////////////////////////
// eNB/UE - API for PHY
////////////////////////////////////////////////////////////////////////////
// /**
// FUNCTION   :: MacLteReceiveTransportBlockFromPhy
// LAYER      :: MAC
// PURPOSE    :: Receive a Transport Block from PHY Layer
// PARAMETERS ::
// + node              : Node*    : Pointer to node.
// + interfaceIndex    : int      : Interface Index
// + srcRnti           : LteRnti  : Source Node's RNTI
// + transportBlockMsg : Message* : one Transport Block
// + isErr             : BOOL     : TRUE:Without ERROR / FALSE:With ERROR
// RETURN     :: void : NULL
// **/
void MacLteReceiveTransportBlockFromPhy(
    Node* node, int interfaceIndex, LteRnti srcRnti,
    Message* transportBlockMsg, BOOL isErr);

////////////////////////////////////////////////////////////////////////////
// eNB - API for PHY
////////////////////////////////////////////////////////////////////////////
// /**
// FUNCTION   :: MacLteNotifyBsrFromPhy
// LAYER      :: MAC
// PURPOSE    :: SAP for BSR Notification from eNB's PHY LAYER
// PARAMETERS ::
// + node           : Node*      : Pointer to node.
// + interfaceIndex : int        : Interface Index
// + bsrInfo        : LteBsrInfo : BSR Info Structure
// RETURN     :: void : NULL
// **/
void MacLteNotifyBsrFromPhy(
    Node* node, int interfaceIndex, const LteBsrInfo& bsrInfo);


////////////////////////////////////////////////////////////////////////////
// eNB/UE - API for RRC
////////////////////////////////////////////////////////////////////////////
// /**
// FUNCTION   :: MacLteNotifyPowerOn
// LAYER      :: MAC
// PURPOSE    :: SAP for Power ON Notification from RRC Layer
// PARAMETERS ::
// + node           : Node* : Pointer to node.
// + interfaceIndex : int   : Interface Index
// RETURN     :: void : NULL
// **/
void MacLteNotifyPowerOn(Node* node, int interfaceIndex);

// /**
// FUNCTION   :: MacLteNotifyPowerOff
// LAYER      :: MAC
// PURPOSE    :: SAP for Power ON Notification from RRC Layer
// PARAMETERS ::
// + node           : Node* : Pointer to node.
// + interfaceIndex : int   : Interface Index
// RETURN     :: void : NULL
// **/
void MacLteNotifyPowerOff(Node* node, int interfaceIndex);


////////////////////////////////////////////////////////////////////////////
// eNB/UE - API for Scheduler
////////////////////////////////////////////////////////////////////////////
// /**
// FUNCTION   :: LteMacCheckMacPduSizeWithoutPadding
// LAYER      :: MAC
// PURPOSE    :: Check MAC PDU acutual size without padding size
// PARAMETERS ::
// + node           : Node*   : Pointer to node.
// + interfaceIndex : int     : Interface Index
// + dstRnti        : LteRnti : Destination RNTI
// + bearerId       : int     : Bearer ID
// RETURN     :: void : NULL
// **/
int LteMacCheckMacPduSizeWithoutPadding(
    Node* node,
    int interfaceIndex,
    const LteRnti& dstRnti,
    const int bearerId,
    int size);

void MacLteClearInfo(Node* node, int interfaceIndex, const LteRnti& rnti);

#endif // _LAYER2_LTE_MAC_H_
