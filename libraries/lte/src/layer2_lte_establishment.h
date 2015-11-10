#ifndef _LAYER2_LTE_ESTABLISHMENT_H_
#define _LAYER2_LTE_ESTABLISHMENT_H_

#include "layer2_lte_mac.h"

// /**
// STRUCT      :: LteRaPreamble
// DESCRIPTION :: Random Access Preamble Info
// **/
typedef struct {
    int raPRACHMaskIndex; // ra-PRACH-MaskIndex [0-15]
                          // Phase1 use only
                          // MAC_LTE_DEFAULT_RA_PRACH_MASK_INDEX
    int raPreambleIndex; // ra-PreambleIndex [0-63]
    double preambleReceivedTargetPower;
    LteRnti ueRnti;
} LteRaPreamble;

// /**
// STRUCT      :: LteRaGrant
// DESCRIPTION :: Random Access Grant Info
// **/
typedef struct {
    LteRnti ueRnti;
} LteRaGrant;


////////////////////////////////////////////////////////////////////////////
// eNB - API for PHY
////////////////////////////////////////////////////////////////////////////
// /**
// FUNCTION   :: MacLteNotifyReceivedRaPreamble
// LAYER      :: MAC
// PURPOSE    :: Notify RA Preamble from eNB's PHY Layer
// PARAMETERS ::
// + node           : Node*             : Pointer to node.
// + interfaceIndex : int               : Interface Index
// + ueRnti     : const LteRnti& ueRnti : UE's RNTI
// RETURN     :: void : NULL
// **/
void MacLteNotifyReceivedRaPreamble(
    Node* node, int interfaceIndex, const LteRnti& ueRnti,
    BOOL isHandingoverRa);



// /**
// FUNCTION   :: RrcLteNotifyRrcConnectionSetupComplete
// LAYER      :: RRC
// PURPOSE    :: Nortify RRCConnectionSetupComplete from eNB's PHY Layer
// PARAMETERS ::
// + node           : Node*   : Pointer to node.
// + interfaceIndex : int     : Interface Index
// + enbRnti        : LteRnti : eNB's RNTI (Destination)
// + ueRnti         : LteRnti : UE's RNTI  (Source)
// RETURN     :: void : NULL
// **/
void RrcLteNotifyRrcConnectionSetupComplete(
    Node* node, int interfaceIndex,
    const LteRnti& enbRnti, const LteRnti& ueRnti);



// /**
// FUNCTION   :: RrcLteNotifyRrcConnectionReconfComplete
// LAYER      :: RRC
// PURPOSE    :: Nortify RRCConnectionReconfComplete from eNB's PHY Layer
// PARAMETERS ::
// + node           : Node*   : Pointer to node.
// + interfaceIndex : int     : Interface Index
// + enbRnti        : LteRnti : eNB's RNTI (Destination)
// + ueRnti         : LteRnti : UE's RNTI  (Source)
// RETURN     :: void : NULL
// **/
void RrcLteNotifyRrcConnectionReconfComplete(
    Node* node, int interfaceIndex,
    const LteRnti& enbRnti, const LteRnti& ueRnti);



////////////////////////////////////////////////////////////////////////////
// eNB - API for MAC
////////////////////////////////////////////////////////////////////////////
// /**
// FUNCTION   :: RrcLteNotifyNetworkEntryFromUE
// LAYER      :: RRC
// PURPOSE    :: Notify network entry from UE
// PARAMETERS ::
// + node           : Node*   : Pointer to node.
// + interfaceIndex : int     : Interface Index
// + ueRnti         : LteRnti : UE's RNTI
// RETURN     :: void : NULL
// **/
void RrcLteNotifyNetworkEntryFromUE(
    Node* node, int interfaceIndex, const LteRnti& ueRnti);

// /**
// FUNCTION   :: MacLteProcessRaGrantWaitingTimerExpired
// LAYER      :: MAC
// PURPOSE    :: Process RA Grant waiting Timer expire event.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + msg              : Message           : Event message
// RETURN     :: void : NULL
// **/
void MacLteProcessRaGrantWaitingTimerExpired(
    Node* node, UInt32 interfaceIndex, Message* msg);

// /**
// FUNCTION   :: MacLteProcessRaBackoffWaitingTimerExpired
// LAYER      :: MAC
// PURPOSE    :: Process RA Backoff waiting Timer expire event.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + msg              : Message           : Event message
// RETURN     :: void : NULL
// **/
void MacLteProcessRaBackoffWaitingTimerExpired(
    Node* node, UInt32 interfaceIndex, Message* msg);

////////////////////////////////////////////////////////////////////////////
// UE - API for RRC
////////////////////////////////////////////////////////////////////////////
// /**
// FUNCTION   :: MacLteStartRandomAccess
// LAYER      :: MAC
// PURPOSE    :: Random Access Start Point
// PARAMETERS ::
// + node           : Node* : Pointer to node.
// + interfaceIndex : int   : Interface Index
// RETURN     :: void : NULL
// **/
void MacLteStartRandomAccess(Node* node, int interfaceIndex);

// RRC ConnectedNotification
// /**
// FUNCTION   :: MacLteNotifyRrcConnected
// LAYER      :: MAC
// PURPOSE    :: Notify RRC_CONNECTED from UE's RRC Layer
// PARAMETERS ::
// + node           : Node* : Pointer to node.
// + interfaceIndex : int   : Interface Index
// + handingover    : BOOL  : whether handingover
// RETURN     :: void : NULL
// **/
void MacLteNotifyRrcConnected(Node* node, int interfaceIndex,
                              BOOL handingover = FALSE);



////////////////////////////////////////////////////////////////////////////
// UE - API for MAC
////////////////////////////////////////////////////////////////////////////
// /**
// FUNCTION   :: RrcLteNotifyRandomAccessResults
// LAYER      :: RRC
// PURPOSE    :: Notify Random Access Results from MAC Layer
// PARAMETERS ::
// + node           : Node*   : Pointer to node.
// + interfaceIndex : int     : Interface Index
// + enbRnti        : LteRnti : eNB's RNTI
// + isSuccess      : BOOL    : TRUE:Success / FALSE:Failure
// RETURN     :: void : NULL
// **/
void RrcLteNotifyRandomAccessResults(
    Node* node, int interfaceIndex, const LteRnti& enbRnti, BOOL isSuccess);


////////////////////////////////////////////////////////////////////////////
// UE - API for PHY
////////////////////////////////////////////////////////////////////////////
// /**
// FUNCTION   :: MacLteNotifyReceivedRaGrant
// LAYER      :: MAC
// PURPOSE    :: Notify RA Grant from UE's PHY Layer
// PARAMETERS ::
// + node           : Node*      : Pointer to node.
// + interfaceIndex : int        : Interface Index
// + raGrant        : LteRaGrant : RA Grant Structure
// RETURN     :: void : NULL
// **/
void MacLteNotifyReceivedRaGrant(
    Node* node, int interfaceIndex, const LteRaGrant& raGrant);

// /**
// FUNCTION   :: MacLteNotifyReceivedRaGrant
// LAYER      :: MAC
// PURPOSE    :: Notify RA Grant from UE's PHY Layer
// PARAMETERS ::
// + node           : Node*   : Pointer to node.
// + interfaceIndex : int     : Interface Index
// + isSuccess      : BOOL    : RA Grant Structure
// + enbRnti        : LteRnti : eNB's RNTI
//                              (if isSuccess == FALSE, do not need enbRnti)
// RETURN     :: void : NULL
// **/
void RrcLteNotifyCellSelectionResults(
    Node* node, int interfaceIndex, BOOL isSuccess,
    const LteRnti& enbRnti = LTE_INVALID_RNTI);

// RRC Connection Setup Complete notification
// /**
// FUNCTION   :: RrcLteNotifyDetectingBetterCell
// LAYER      :: RRC
// PURPOSE    :: Notify Detecting Better Cell from PHY Layer
// PARAMETERS ::
// + node           : Node*   : Pointer to node.
// + interfaceIndex : int     : Interface Index
// + enbRnti        : LteRnti : eNB's RNTI
// RETURN     :: void : NULL
// **/
void RrcLteNotifyDetectingBetterCell(
    Node* node, int interfaceIndex, const LteRnti& enbRnti);


// /**
// FUNCTION   :: RrcLteRandomAccessForHandoverExecution
// LAYER      :: RRC
// PURPOSE    :: random access for handover execution
// PARAMETERS ::
// + node           : Node*   : Pointer to node.
// + interfaceIndex : int     : Interface Index
// + enbRnti        : LteRnti : target eNB's RNTI
// RETURN     :: void : NULL
// **/
void RrcLteRandomAccessForHandoverExecution(
    Node* node, int interfaceIndex, const LteRnti& enbRnti);



#endif // _LAYER2_LTE_ESTABLISHMENT_H_
