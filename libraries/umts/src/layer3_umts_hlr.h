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

#ifndef _LAYER3_UMTS_HLR_H_
#define _LAYER3_UMTS_HLR_H_

//--------------------------------------------------------------------------
// main data structure for UMTS layer3 implementation of HLR
//--------------------------------------------------------------------------

// /**
// STRUCT      :: UmtsHlrEntry
// DESCRIPTION :: One entry which stores information of a UE in the HLR
// **/
typedef struct umts_hlr_entry_str
{
    // following are two keys
    UInt32 hashKey;      // Hash key which is UE ID, uniquelly global
    Address nodeAddr;    // Network address associated with the UE

    // basic info of the UE
    CellularIMSI imsi;   // IMSI of the UE, stores home MCC and MNC
    UInt32 tmsi;         // TMSI
    UInt32 lmsi;         // LMSI

    // current location/routing area
    CellularMCC mcc;     // MCC of current area
    CellularMNC mnc;     // MNC of current area
    UInt32 sgsnId;       // node ID of current SGSN
    Address sgsnAddr;    // Address of he SGSN node
} UmtsHlrEntry;

// /**
// MAP         :: UmtsHlrMap
// DESCRIPTION :: A hash key and UmtsHlrEntry mapping
// **/
typedef std::map<UInt32, UmtsHlrEntry*> UmtsHlrMap;

//--------------------------------------------------------------------------
// Type definitions, data structures for interaction between HLR and
// VLR/SGSN/GGSN
//--------------------------------------------------------------------------

// /**
// ENUM        :: UmtsHlrCmdTgt
// DESCRIPTION :: Target of command can be sent to HLR
// **/
typedef enum
{
    UMTS_HLR_COMMAND_MS  = 3,       // target is mobile station
    UMTS_HLR_COMMAND_CELL,          // target is cell
} UmtsHlrCmdTgt;

// /**
// ENUM        :: UmtsHlrCommand
// DESCRIPTION :: Type of command can be sent to HLR
// **/
typedef enum
{
    UMTS_HLR_UPDATE,       // Update or add entry in HLR
    UMTS_HLR_REMOVE,       // Remove an entry from HLR if exist
    UMTS_HLR_QUERY,        // Look up an UE in HLR
    UMTS_HLR_UPDATE_REPLY, // Reply to UPDATE request
    UMTS_HLR_REMOVE_REPLY, // Reply to REMOVE request
    UMTS_HLR_QUERY_REPLY,  // Reply of lookup query
    UMTS_HLR_UPDATE_CELL,  // Update cell information
    UMTS_HLR_UPDATE_CELL_REPLY,
    UMTS_HLR_QUERY_CELL,   // Query cell information 
    UMTS_HLR_QUERY_CELL_REPLY,
    UMTS_HLR_RNC_UPDATE // HLR Updates RNC info in the SGSN
} UmtsHlrCommand;

// /**
// ENUM        :: UmtsHlrCommandStatus
// DESCRIPTION :: Status of a HLR request command in reply message
// **/
typedef enum
{
    UMTS_HLR_COMMAND_SUCC,  // successful
    UMTS_HLR_COMMAND_FAIL,  // failed
} UmtsHlrCommandStatus;

// /**
// STRUCT      :: UmtsHlrLocationInfo
// DESCRIPTION :: Location info of an UE. May present in some HLR related
//                requests and replies
// **/
typedef struct umts_hlr_location_info_str
{
    // location info
    CellularMCC mcc;    // MCC of current area
    CellularMNC mnc;    // MNC of current area
    UInt32 sgsnId;      // Current serving SGSN node
    Address sgsnAddr;   // Address of SGSN node
} UmtsHlrLocationInfo;

// /**
// STRUCT      :: UmtsHlrRequest
// DESCRIPTION :: A request message sent from other entities to HLR
// **/
typedef struct umts_hlr_request_str
{
    unsigned char command;  // in the type of UmtsHlrCommand
    unsigned char imsi[CELLULAR_COMPACT_IMSI_LENGTH]; // identity of the UE
} UmtsHlrRequest;

// /**
// STRUCT      :: UmtsHlrReply
// DESCRIPTION :: A reply message sent from HLR to other entities
// **/
typedef struct umts_hlr_reply_str
{
    unsigned char command;  // in the type of UmtsHlrCommand
    unsigned char result;   // in the type UmtsHlrCommandStatus
    unsigned char imsi[CELLULAR_COMPACT_IMSI_LENGTH]; // identity of the UE
} UmtsHlrReply;

// /**
// STRUCT      :: UmtsLayer3HlrStats
// DESCRIPTION :: Statistics collected at the HLR
// **/
typedef struct umts_layer3_hlr_stat_str
{
    UInt32 numRoutingUpdateRcvd;
    UInt32 numRoutingQueryRcvd;
    UInt32 numRoutingQuerySuccReplySent;
    UInt32 numRoutingQueryFailReplySent;
} UmtsLayer3HlrStat;

// /**
// STRUCT      :: UmtsLayer3Hlr
// DESCRIPTION :: Structure of the HLR specific layer 3 data UMTS HLR node
// **/
typedef struct umts_layer3_hlr_str
{
    UInt32 hlrNumber;  // my HLR number. Equal to my node ID
    UmtsHlrMap *hlrMap;  // Data map of the HLR
    UmtsCellCacheMap* cellCacheMap;     //CELL ID/Cached info map
    UmtsRncCacheMap* rncCacheMap;

    UmtsLayer3HlrStat stat;
} UmtsLayer3Hlr;

//--------------------------------------------------------------------------
//  API functions
//--------------------------------------------------------------------------

// /**
// FUNCTION   :: UmtsLayer3HlrHandlePacket
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
void UmtsLayer3HlrHandlePacket(Node *node,
                               Message *msg,
                               UmtsLayer3Data *umtsL3,
                               int interfaceIndex,
                               Address srcAddr);

// /**
// FUNCTION   :: UmtsLayer3HlrInit
// LAYER      :: Layer 3
// PURPOSE    :: Initialize HLR data at UMTS layer 3 data.
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + nodeInput : const NodeInput* : Pointer to node input.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// RETURN     :: void : NULL
// **/
void UmtsLayer3HlrInit(Node *node,
                       const NodeInput *nodeInput,
                       UmtsLayer3Data *umtsL3);

// /**
// FUNCTION   :: UmtsLayer3HlrLayer
// LAYER      :: Layer 3
// PURPOSE    :: Handle HLR specific timers and layer messages.
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + msg       : Message*         : Message for node to interpret
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// RETURN     :: void : NULL
// **/
void UmtsLayer3HlrLayer(Node *node, Message *msg, UmtsLayer3Data *umtsL3);

// /**
// FUNCTION   :: UmtsLayer3HlrFinalize
// LAYER      :: Layer 3
// PURPOSE    :: Print HLR specific stats and clear protocol variables.
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// RETURN     :: void : NULL
// **/
void UmtsLayer3HlrFinalize(Node *node, UmtsLayer3Data *umtsL3);

#endif /* _LAYER3_UMTS_HLR_H_ */
