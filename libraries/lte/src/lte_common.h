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

// /**
// PACKAGE     :: LTE_COMMON
// DESCRIPTION :: Common function in LTE Model
//                This file contains some default cross-layer configuration
// **/

#ifndef LTE_COMMON
#define LTE_COMMON

#include <list>
#include <limits.h>

#ifdef ADDON_DB
#include "dbapi.h"
#endif // ADDON_DB

// [S] For HO Validation ----------------------------------------------------
#ifdef LTE_LIB_LOG
#ifdef LTE_LIB_HO_VALIDATION
const UInt32 LTE_LIB_HO_VALIDATION_NUM_MAX_PARTITION(64);
typedef struct struct_enb_pos {
    double x;
    double y;
    double z;

    struct_enb_pos(double X, double Y, double Z)
        : x(X), y(Y), z(Z) {}
} EnbPos;
typedef std::map<int, const EnbPos> EnbPosMap; // first is nodeId

// distance
#define LteCalcDistance(A, B) \
    sqrt(pow(((A).x-(B).x),2)+pow(((A).y-(B).y),2)+pow(((A).z-(B).z),2))

#endif
#endif
// [E] For HO Validation ----------------------------------------------------

///////////////////////////////////////////////////////////////
// MACRO to enable and disable the following feature
///////////////////////////////////////////////////////////////
#define ENABLE_TEST_SCHEDULER

///////////////////////////////////////////////////////////////
// define name
///////////////////////////////////////////////////////////////
#define LTE_STRING_STATION_TYPE_ENB "eNB"
#define LTE_STRING_STATION_TYPE_UE "UE"
//-------------------------------------
//#define LTE_STRING_LAYER_TYPE_PROP "PROP"
//#define LTE_STRING_LAYER_TYPE_PHY "PHY"
//#define LTE_STRING_LAYER_TYPE_MAC "MAC"
//#define LTE_STRING_LAYER_TYPE_RLC "RLC"
//#define LTE_STRING_LAYER_TYPE_PDCP "PDCP"
//#define LTE_STRING_LAYER_TYPE_RRC "RRC"
//#define LTE_STRING_LAYER_TYPE_SCHEDULER "SCH"
//-------------------------------------
#define LTE_STRING_CATEGORY_TYPE_POWER_ON "PowerOn"
#define LTE_STRING_CATEGORY_TYPE_POWER_OFF "PowerOff"
//-----------------
#define LTE_STRING_CATEGORY_TYPE_SET_STATE "SetState"
//-----------------
#define LTE_STRING_CATEGORY_TYPE_CELL_SELECTION "CellSelection"
#define LTE_STRING_CATEGORY_TYPE_RANDOM_ACCESS "RandomAccess"
#define LTE_STRING_CATEGORY_TYPE_ESTABLISHMENT "Establishment"
#define LTE_STRING_CATEGORY_TYPE_CONNECTION "Connection"
//-----------------
#define LTE_STRING_CATEGORY_TYPE_RA_PREAMBLE_TX "RaPreambleTx"
#define LTE_STRING_CATEGORY_TYPE_RA_PREAMBLE_RX "RaPreambleRx"
#define LTE_STRING_CATEGORY_TYPE_CQI_TX "CqiTx"
#define LTE_STRING_CATEGORY_TYPE_CQI_RX "CqiRx"
#define LTE_STRING_CATEGORY_TYPE_SRS_TX "SrsTx"
#define LTE_STRING_CATEGORY_TYPE_SRS_RX "SrsRx"
//-----------------
#define LTE_STRING_CATEGORY_TYPE_BSR "BSR"
#define LTE_STRING_CATEGORY_TYPE_CQI "CQI"
#define LTE_STRING_CATEGORY_TYPE_SRS "SRS"
#define LTE_STRING_CATEGORY_TYPE_RI "RI"
#define LTE_STRING_CATEGORY_TYPE_MIMO "MIMO"
#define LTE_STRING_CATEGORY_TYPE_TX_POWER_RB "TxPowerRb"
#define LTE_STRING_CATEGORY_TYPE_UL_TX_POWER_RB_DETAIL "UlTxPowerRbDetail"
#define LTE_STRING_CATEGORY_TYPE_RB_RX_POWER "RbRxPower"
#define LTE_STRING_CATEGORY_TYPE_PHY_STATE "PhyState"
#define LTE_STRING_CATEGORY_TYPE_RB_THERMAL_NOISE "RbThermalNoise"
#define LTE_STRING_CATEGORY_TYPE_PATHLOSS "Ploss"
#define LTE_STRING_CATEGORY_TYPE_DL_PATHLOSS "DlPloss"
#define LTE_STRING_CATEGORY_TYPE_UL_PATHLOSS "UlPloss"
#define LTE_STRING_CATEGORY_TYPE_RSRP "Rsrp"
#define LTE_STRING_CATEGORY_TYPE_RSRQ "Rsrq"
#define LTE_STRING_CATEGORY_TYPE_PPER "PperCalculation"
#define LTE_STRING_CATEGORY_TYPE_CHRSP "ChRsp"
#define LTE_STRING_CATEGORY_TYPE_IFPOWER "IfPower"
#define LTE_STRING_CATEGORY_TYPE_TB_SINR "TbSinr"
#define LTE_STRING_CATEGORY_TYPE_RB_SINR "RbSinr"
#define LTE_STRING_CATEGORY_TYPE_TB_SINR_PDF "TbSinrPdf"
#define LTE_STRING_CATEGORY_TYPE_DL_ESTIMATED_TB_SINR_PDF \
                                                    "DlEstimatedTbSinrPdf"
#define LTE_STRING_CATEGORY_TYPE_UL_ESTIMATED_TB_SINR_PDF \
                                                    "UlEstimatedTbSinrPdf"
#define LTE_STRING_CATEGORY_TYPE_CQI_TB_SINR_PDF "CqiTbSinrPdf"
#define LTE_STRING_CATEGORY_TYPE_CQI_PDF "CqiPdf"
#define LTE_STRING_CATEGORY_TYPE_DL_MCS_PDF "DlMcsPdf"
#define LTE_STRING_CATEGORY_TYPE_UL_MCS_PDF "UlMcsPdf"
#define LTE_STRING_CATEGORY_TYPE_PPER_PDF "PperPdf"
#define LTE_STRING_CATEGORY_TYPE_LOST_DETECTION "LostDetection"
#define LTE_STRING_CATEGORY_TYPE_BETTER_CELL "BetterCell"
//-----------------
#define LTE_STRING_CATEGORY_TYPE_RLC_TRANSMITTION "TRANSMITTION"
#define LTE_STRING_CATEGORY_TYPE_RLC_RESEPTION "RESEPTION"
#define LTE_STRING_CATEGORY_TYPE_RLC_CONTROL "CONTROL"
#define LTE_STRING_CATEGORY_TYPE_RLC_CREATE_STATUS_PDU "CreateStatusPdu"
#define LTE_STRING_CATEGORY_TYPE_RLC_NOT_CREATE_STATUS_PDU \
                                                        "NotCreateStatusPdu"
#define LTE_STRING_CATEGORY_TYPE_RLC_DEQUEUE_INFO "DequeueInfo"

//- Log Categories for SCH layer -------------------------------------
#define LTE_STRING_CATEGORY_TYPE_CONNECTED_UE "ConnectedUE"
#define LTE_STRING_CATEGORY_TYPE_CONNECTED_UE_DL "ConnectedUeDl"
#define LTE_STRING_CATEGORY_TYPE_CONNECTED_UE_UL "ConnectedUeUl"
#define LTE_STRING_CATEGORY_TYPE_TARGET_UE_DL "TargetUeDl"
#define LTE_STRING_CATEGORY_TYPE_TARGET_UE_UL "TargetUeUl"
#define LTE_STRING_CATEGORY_TYPE_RAW_RESULT_DL "RawResultDl"
#define LTE_STRING_CATEGORY_TYPE_RAW_RESULT_UL "RawResultUl"
#define LTE_STRING_CATEGORY_TYPE_RESULT_DL "ResultDl"
#define LTE_STRING_CATEGORY_TYPE_RESULT_UL "ResultUl"

#define LTE_STRING_CATEGORY_TYPE_RRC_TIMER "RRCTimer"
#define LTE_STRING_CATEGORY_TYPE_CONNECTION_INFO "ConnectionInfo"
#define LTE_STRING_CATEGORY_TYPE_ROUTING "Routing"


//-------------------------------------
#define LTE_STRING_COMMON_TYPE_BEFORE "Before"
#define LTE_STRING_COMMON_TYPE_AFTER "After"
#define LTE_STRING_COMMON_TYPE_ADD "Add"
#define LTE_STRING_COMMON_TYPE_REMOVE "Remove"
//-------------------------------------
#define LTE_STRING_PROCESS_TYPE_START "Start"
#define LTE_STRING_PROCESS_TYPE_RESTART "Restart"
//-------------------------------------
#define LTE_STRING_RESULT_TYPE_SUCCESS "Success"
#define LTE_STRING_RESULT_TYPE_FAILURE "Failure"
//-------------------------------------
#define LTE_STRING_MESSAGE_TYPE_RA_PREAMBLE "RaPreamble"
#define LTE_STRING_MESSAGE_TYPE_RA_GRANT "RaGrant"
#define LTE_STRING_MESSAGE_TYPE_RRC_CONNECTION_SETUP_COMPLETE \
    "RrcConnectionSetupComplete"
//-------------------------------------
#define LTE_STRING_FORMAT_RNTI "[%d %d]"
#define LTE_STRING_FORMAT_HANDOVER_PARTICIPATOR \
    "UE:[%d %d],SRC:[%d %d],TGT:[%d %d]"

///////////////////////////////////////////////////////////////
// define
///////////////////////////////////////////////////////////////
#define LTE_INFINITY_POWER_dBm            (1000.0)
#define LTE_NEGATIVE_INFINITY_POWER_dBm   (-1000.0)
#define LTE_INFINITY_SINR_dB              (1000.0)
#define LTE_NEGATIVE_INFINITY_SINR_dB     (-1000.0)
#define LTE_INFINITY_PATHLOSS_dB          (1000.0)
#define LTE_NEGATIVE_INFINITY_PATHLOSS_dB (-1000.0)
#define LTE_INVALID_RSRP_dBm              (-1000.0)
#define LTE_INVALID_RSRQ_dB               (-1000.0)

#define LTE_MAX_NUM_RB (100) //same as
                        // PHY_LTE_CHANNEL_BANDWIDTH_20MHZ_NUM_RB @ lte_phy.h

// invalid RNTI
#define LTE_INVALID_RNTI (LteRnti(ANY_ADDRESS, ANY_INTERFACE))
#define LTE_FRAME_LENGTH (10*MILLI_SECOND) // 1Frame Length[nsec]
#define LTE_IPV4_UNKNOWN_ADDRESS (0x00000000)

///////////////////////////////////////////////////////////////
// Macros
///////////////////////////////////////////////////////////////
#define MATRIX_INDEX(r,c,col_size) ((r)*(col_size)+(c))
#define IS_INVALID_HANDOVER_PARTICIPATOR(hop) \
    (hop.ueRnti == LTE_INVALID_RNTI || hop.srcEnbRnti == LTE_INVALID_RNTI \
    || hop.tgtEnbRnti == LTE_INVALID_RNTI)

///////////////////////////////////////////////////////////////
// typedef
///////////////////////////////////////////////////////////////
// 1st:EventType, 2nd:TimerMessage
typedef std::pair < int, Message* > LtePairMessage;
typedef std::map < int, Message* > LteMapMessage;

///////////////////////////////////////////////////////////////
// typedef enum
///////////////////////////////////////////////////////////////

// /**
// ENUM        :: LteStationType
// DESCRIPTION :: Type of a station
// **/
typedef enum
{
    LTE_STATION_TYPE_ENB,      // evolutional Node B (eNB)
    LTE_STATION_TYPE_UE       // User Equipment (UE)
} LteStationType;

// /**
// ENUM        :: LteTxScheme
// DESCRIPTION :: Transmission scheme
// **/
typedef enum
{
    TX_SCHEME_SINGLE_ANTENNA,
    TX_SCHEME_DIVERSITY,
    TX_SCHEME_OL_SPATIAL_MULTI,
    TX_SCHEME_INVALID,
} LteTxScheme;

///////////////////////////////////////////////////////////////
// typedef struct
///////////////////////////////////////////////////////////////
// /**
// STRUCT      :: LteRnti
// DESCRIPTION :: LteRnti Structure
// **/
typedef struct lte_rnti
{
    NodeAddress nodeId;
    int interfaceIndex;

    lte_rnti () : nodeId(ANY_ADDRESS), interfaceIndex(ANY_INTERFACE){}

    lte_rnti (NodeAddress node_id, int if_id)
    {
        nodeId = node_id;
        interfaceIndex = if_id;
    }

    bool operator < (const lte_rnti& a) const
    {
        if (nodeId < a.nodeId ||
            (nodeId == a.nodeId && interfaceIndex < a.interfaceIndex))
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool operator == (const lte_rnti& a) const
    {
        return nodeId == a.nodeId && interfaceIndex == a.interfaceIndex
            ? true : false;
    }

    bool operator != (const lte_rnti& a) const
    {
        return nodeId != a.nodeId || interfaceIndex != a.interfaceIndex
            ? true : false;
    }
} LteRnti;

// List of RNTI
typedef std::list < LteRnti > ListLteRnti;

typedef struct struct_handover_participator
{
    LteRnti ueRnti;
    LteRnti srcEnbRnti;
    LteRnti tgtEnbRnti;
    struct_handover_participator()
        : ueRnti(LTE_INVALID_RNTI),
        srcEnbRnti(LTE_INVALID_RNTI),
        tgtEnbRnti(LTE_INVALID_RNTI)
    {}
    struct_handover_participator(
        LteRnti ueRnti,
        LteRnti srcEnbRnti,
        LteRnti tgtEnbRnti)
        : ueRnti(ueRnti),
        srcEnbRnti(srcEnbRnti),
        tgtEnbRnti(tgtEnbRnti)
    {}
} HandoverParticipator;

// /**
// STRUCT:: LteDciFormat0
// DESCRIPTION:: DCI Format0 (for UL-Tx)
// **/
typedef struct struct_lte_dci_format0_str
{
    UInt8 resAllocHeader;
    bool freqHopFlag;
    //UInt8 usedRB_list[LTE_MAX_NUM_RB];
    UInt8 usedRB_start;
    UInt8 usedRB_length;
    UInt8 mcsID; // McsIndex
} LteDciFormat0;

// /**
// STRUCT:: LteDciFormat1
// DESCRIPTION:: DCI Format1 (for DL-Tx, Single or Tx-Diversity)
// **/
typedef struct struct_lte_dci_format1_str
{
    UInt8 resAllocHeader;
    UInt8 usedRB_list[LTE_MAX_NUM_RB];
    UInt8 mcsID; // McsIndex
} LteDciFormat1;

// /**
// STRUCT:: PhyLteDciFormat2a TB
// DESCRIPTION:: DCI Format2a TB
// **/
typedef struct struct_lte_dci_format2a_For_Tb_str
{
    UInt8 mcsID; // McsIndex
} LteDciFormat2aTb;

// /**
// STRUCT:: LteDciFormat2a
// DESCRIPTION:: DCI Format2a (for OL-Spatial-multiplexing)
// **/
typedef struct struct_lte_dci_format2a_str
{
    UInt8 resAllocHeader;
    UInt8 usedRB_list[LTE_MAX_NUM_RB];
    bool tb2cwSwapFlag;
    //UInt8 mcsIDforTB1; // McsIndex for TB1
    //UInt8 mcsIDforTB2; // McsIndex for TB2
    LteDciFormat2aTb forTB[2]; // for TB1,TB2
} LteDciFormat2a;


#ifdef ADDON_DB
// /**
// STRUCT:: LteStatsDbSduPduDataCtrlInfo
// DESCRIPTION:: Used by Stats-DB to transfer data & control info
// **/
typedef struct struct_lte_stats_db_sdu_pdu_info
{
    UInt32 dataSize;
    UInt32 ctrlSize;
    BOOL   isCtrlPacket;
} LteStatsDbSduPduInfo;
#endif

//--------------------------------------------------------------------------
//  API functions
//--------------------------------------------------------------------------

// /**
// FUNCTION   :: LteGetTxSchemeString
// LAYER      :: LTE
// PURPOSE    :: get the tx scheme as the string.
// PARAMETERS ::
// + mode      : LteTxScheme   : tx scheme.
// RETURN     :: char* : the string of tx scheme.
// **/
const char* LteGetTxSchemeString(LteTxScheme mode);

// /**
// FUNCTION   :: LteFreeMsg
// LAYER      :: LTE
// PURPOSE    :: free message
// PARAMETERS ::
// + node        : Node*         : pointer to Node
// + msg         : Message**     : message should be deleted
// RETURN     :: void : NULL
// **/
void LteFreeMsg(
        Node* node,
        Message** msg);



// /**
// FUNCTION   :: LteAddMsgInfo
// LAYER      :: LTE
// PURPOSE    :: add message info
// PARAMETERS ::
// + node           : Node*             : pointer to the network node
// + interfaceIndex : int               : index of interface
// + msg            : Message*          : message
// + infoType       : const int         : infoType to be set
// + info           : T&                : reference to info
// RETURN     :: void : NULL
// **/
template<typename T>
void LteAddMsgInfo(
        Node* node,
        int interfaceIndex,
        Message* msg,
        unsigned short infoType,
        T& info)
{
    ERROR_Assert(msg, "msg is NULL");
    const int infoLength = (int) sizeof(T);
    T* infoHead = (T*) MESSAGE_AddInfo(node, msg, infoLength, infoType);
    ERROR_Assert(infoHead, "MESSAGE_AddInfo is failed.");
    memcpy(infoHead, &info, infoLength);
}



// /**
// FUNCTION   :: LteAddRouteToForwardingTable
// LAYER      :: LTE
// PURPOSE    :: add route to forwarding table of QualNet
// PARAMETERS ::
// + node                  : Node*         : pointer to Node
// + destAddr              : NodeAddress   : destination address
// + destMask              : NodeAddress   : destination mask
// + nextHop               : NodeAddress   : next hop
// + outgoingInterfaceIndex: int           : outgoing interface index
// RETURN     :: void : NULL
// **/
void
LteAddRouteToForwardingTable(
       Node *node,
       NodeAddress destAddr,
       NodeAddress destMask,
       NodeAddress nextHop,
       int outgoingInterfaceIndex);
// /**
// FUNCTION   :: LteDeleteRouteFromForwardingTable
// LAYER      :: LTE
// PURPOSE    :: delete route from forwarding table of QualNet
// PARAMETERS ::
// + node                  : Node*         : pointer to Node
// + destAddr              : NodeAddress   : destination address
// + destMask              : NodeAddress   : destination mask
// RETURN     :: void : NULL
// **/
void
LteDeleteRouteFromForwardingTable(
       Node *node,
       NodeAddress destAddr,
       NodeAddress destMask);

// /**
// FUNCTION   :: LteGetPhyIndexFromMacInterfaceIndex
// LAYER      :: LTE
// PURPOSE    :: Get PhyIndex(PhyNumber) from MAC I/F index
// PARAMETERS ::
// + node                  : Node*         : pointer to Node
// + interfaceIndex        : int           : LTE's MAC I/F index
// RETURN     :: int : PhyIndex
//               if there is no LTE PHY, return ANY_INTERFACE
// **/
int LteGetPhyIndexFromMacInterfaceIndex(
       Node *node,
       int interfaceIndex);

// /**
// FUNCTION   :: LteGetMacInterfaceIndexFromPhyIndex
// LAYER      :: LTE
// PURPOSE    :: Get MAC I/F index from PhyIndex(PhyNumber)
// PARAMETERS ::
// + node                  : Node*         : pointer to Node
// + phyIndex              : int           : LTE's PhyIndex
// RETURN     :: int : MAC I/F index
//               if there is no LTE MAC, return ANY_INTERFACE
// **/
int LteGetMacInterfaceIndexFromPhyIndex(
       Node *node,
       int phyIndex);

#ifdef ADDON_DB
// /**
// FUNCTION   :: LteMacStatsDBCheckForMsgSequenceNum
// LAYER      :: MAC
// PURPOSE    :: Check for msg seq num, if found 0 then provide a valid value
//               Additionally, should check for msg Id. Add it if not exist.
// PARAMETERS ::
// + node               : IN : Node*    : Pointer to node.
// + msg                : IN : Message* : PDU/SDU Message Structure
// RETURN     :: void   : NULL
// **/
void LteMacStatsDBCheckForMsgSequenceNum(Node* node,
                                         Message* msg);


// /**
// FUNCTION   :: LteMacAddStatsDBInfo
// LAYER      :: MAC
// PURPOSE    :: Add StatsDB Info
// PARAMETERS ::
// + node               : IN : Node*    : Pointer to node.
// + msg                : IN : Message* : PDU/SDU Message Structure
// RETURN     :: void   : NULL
// **/
void LteMacAddStatsDBInfo(Node* node,
                          Message* msg);


// /**
// FUNCTION   :: LteMacRemoveStatsDBInfo
// LAYER      :: MAC
// PURPOSE    :: Remove StatsDB Info
// PARAMETERS ::
// + node               : IN : Node*    : Pointer to node.
// + msg                : IN : Message* : PDU/SDU Message Structure
// RETURN     :: void   : NULL
// **/
void LteMacRemoveStatsDBInfo(Node* node,
                          Message* msg);


// /**
// FUNCTION   :: LteMacUpdateStatsDBInfo
// LAYER      :: MAC
// PURPOSE    :: Update StatsDB Info
// PARAMETERS ::
// + node               : IN : Node*    : Pointer to node.
// + msg                : IN : Message* : PDU/SDU Message Structure
// + dataSize           : IN : UInt32   : data size
// + ctrlSize           : IN : UInt32   : control size
// + isCtrlPacket       : IN : BOOL     : 'TRUE' for control packet
// RETURN     :: void   : NULL
// **/
void LteMacUpdateStatsDBInfo(Node* node,
                             Message* msg,
                             UInt32 dataSize,
                             UInt32 ctrlSize,
                             BOOL isCtrlPacket);


// /**
// FUNCTION   :: LteMacAppendStatsDBControlInfo
// LAYER      :: MAC
// PURPOSE    :: Append StatsDB Control Size Info
// PARAMETERS ::
// + node               : IN : Node*    : Pointer to node.
// + msg                : IN : Message* : PDU/SDU Message Structure
// + ctrlSize           : IN : UInt32   : control size
// RETURN     :: void   : NULL
// **/
void LteMacAppendStatsDBControlInfo(Node* node,
                                    Message* msg,
                                    UInt32 ctrlSize);


// /**
// FUNCTION   :: LteMacReduceStatsDBControlInfo
// LAYER      :: MAC
// PURPOSE    :: Substract StatsDB Control Size Info
// PARAMETERS ::
// + node               : IN : Node*    : Pointer to node.
// + msg                : IN : Message* : PDU/SDU Message Structure
// + ctrlSize           : IN : UInt32   : control size
// RETURN     :: void   : NULL
// **/
void LteMacReduceStatsDBControlInfo(Node* node,
                                    Message* msg,
                                    UInt32 ctrlSize);


// /**
// FUNCTION   :: LteMacSetStatsDBInfoInFragMsg
// LAYER      :: MAC
// PURPOSE    :: Reset StatsDB Info in the Fragmented Messages
// PARAMETERS ::
// + node               : IN : Node*    : Pointer to node.
// + msg                : IN : Message* : Original message pointer
// + fragHead           : IN : Message* : First fragemented message pointer
// + fragTail           : IN : Message* : Second fragemented Message pointer
// + restSize           : IN : UInt32   : Size limt for fragHead
// + dataSizeHead       : IN : UInt32&  : Return data size of final fragHead
// + ctrlSizeHead       : IN : UInt32&  : Return ctrl size of final fragHead
// + addStatsDbInfo     : IN : BOOL     : if TRUE, then add info to frag msgs
//                                        Default value: FALSE
// RETURN     :: void   : NULL
// **/
void LteMacSetStatsDBInfoInFragMsg(Node* node,
                                   Message* msg,
                                   Message* fragHead,
                                   Message* fragTail,
                                   UInt32 restSize,
                                   UInt32& dataSizeHead,
                                   UInt32& ctrlSizeHead,
                                   BOOL addStatsDbInfo = FALSE);
#endif // ADDON_DB
#endif //LTE_COMMON
