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

#ifndef _LAYER3_UMTS_NODEB_H_
#define _LAYER3_UMTS_NODEB_H_

// /**
// STRUCT      :: UmtsLayer3NodebPara
// DESCRIPTION :: Parameters of an UMTS NodeB. Some may from RNC
// **/
typedef struct umts_layer3_nodeb_para_str
{
    // systeme info block
    clocktype sysInfoBcstTime; 

    // parameters which control cell selection and reselection
    double Qrxlevmin;       // minimum RX level for cell selection
    double Ssearch;         // Threshold to trigger cell search
    double Qhyst;           // Hysteresis for cell reselection
    clocktype cellEvalTime; // Interval for evaluate a cell

    // parameters which control handover
    double shoAsTh;        // Threshold to add a cell into the AS
    double shoAsThHyst;    // Hysteresis for the AS threshold
    double shoAsRepHyst;   // Replacement hysteresis for AS
    int shoAsSizeMax;      // Maximum size of the AS set
    clocktype shoEvalTime; // Interval for evalulate a cell for SHO
} UmtsLayer3NodebPara;

// /**
// STRUCT      :: UmtsLayer3NodebStat
// DESCRIPTION :: NodeB specific statistics
// **/
typedef struct umts_layer3_nodeb_stat_str
{
    int minNumMobileServed;
    int maxNumMobileServed;
    int numMobileServed;
    int avgNumServingMobile; // average number of 
    clocktype avgMobileStayTime;
    int numUeInConnected;

    // dynamic statistics
    int numUeInConnectedGuiId; 
} UmtsLayer3NodebStat;

// /**
// STRUCT      :: UmtsLayer3CellSwitchTimerInfo
// DESCRIPTION :: Cell Switch timer info 
// **/
struct UmtsLayer3CellSwitchTimerInfo
{
    NodeId ueId;
    UInt32 uePrimScCode;
};

//--------------------------------------------------------------------------
// main data structure for UMTS layer3 implementation of NodeB
//--------------------------------------------------------------------------

// /**
// STRUCT      :: UmtsLayer3Nodeb
// DESCRIPTION :: Structure of the UMTS NodeB node Layer3 data
// **/
typedef struct struct_umts_layer3_nodeb_str
{
    // some system config related info
    BOOL configured;
    UmtsPlmnType plmnType;
    UmtsPlmnIdentity plmnId;
    unsigned char mibValue;
    UInt32  cellId;
    UInt16 regAreaId;
    UmtsDuplexMode duplexMode;
    BOOL edchCapable;
    // HSDPA
    BOOL hsdpaEnabled;
    
    UmtsLayer3NodebPara para;
    UmtsLayer3NodebStat stat;
    UmtsLayer3RncInfo rncInfo;

    Message* sysInfoTimer;
    unsigned int pccpchSfn;
    int sibSchCount;

    // AICH info
    // PICH Info
    // PRACH info
    // PCCPCH info
    // SCcPCH info

    Int64 linkBandWidth;
    int cellSwitchFragSize;
} UmtsLayer3Nodeb;


//--------------------------------------------------------------------------
//  API functions
//--------------------------------------------------------------------------
// /**
// FUNCTION   :: UmtsLayer3NodebReportAmRlcError
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
void UmtsLayer3NodebReportAmRlcError(Node *node,
                                     UmtsLayer3Data*,
                                     NodeId ueId,
                                     char rbId);

// /**
// FUNCTION   :: UmtsLayer3NodebReceivePacketFromMacLayer
// LAYER      :: Layer3
// PURPOSE    :: Handle timers and layer messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface from which the 
//                                          packet is received
// + umtsL3           : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// + msg              : Message*          : Message for node to interpret.
// + lastHopAddress   : NodeAddress       : Address of the last hop
// RETURN     :: void : NULL
// **/
void UmtsLayer3NodebReceivePacketFromMacLayer(
    Node *node,
    int interfaceIndex,
    UmtsLayer3Data* umtsL3,
    Message *msg,
    NodeAddress lastHopAddress);

// /**
// FUNCTION   :: UmtsLayer3NodebHandlePacket
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
void UmtsLayer3NodebHandlePacket(Node *node,
                                 Message *msg,
                                 UmtsLayer3Data *umtsL3,
                                 int interfaceIndex,
                                 Address srcAddr);

//  /** 
// FUNCITON   :: UmtsLayer3NodebHandleInterLayerCommand
// LAYER      :: UMTS L3 at NodeB
// PURPOSE    :: Handle Interlayer command 
// PARAMETERS :: 
// + node             : Node*             : Pointer to node.
// + cmdType          : UmtsInterlayerCommandType : command type
// + interfaceIdnex   : UInt32            : interface index of UMTS
// + cmd              : void*             : cmd to handle
// RETURN     :: void : NULL
void UmtsLayer3NodebHandleInterLayerCommand(
            Node* node,
            UInt32 interfaceIndex,
            UmtsInterlayerCommandType cmdType,
            void* cmd);

// /**
// FUNCTION   :: UmtsLayer3NodebInit
// LAYER      :: Layer 3
// PURPOSE    :: Initialize NodeB data at UMTS layer 3 data.
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + nodeInput : const NodeInput* : Pointer to node input.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// RETURN     :: void : NULL
// **/
void UmtsLayer3NodebInit(Node* node,
                         const NodeInput *nodeInput,
                         UmtsLayer3Data *umtsL3);

// /**
// FUNCTION   :: UmtsLayer3NodebLayer
// LAYER      :: Layer 3
// PURPOSE    :: Handle NodeB specific timers and layer messages.
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + msg       : Message*         : Message for node to interpret
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// RETURN     :: void : NULL
// **/
void UmtsLayer3NodebLayer(Node *node, Message *msg, UmtsLayer3Data *umtsL3);

// /**
// FUNCTION   :: UmtsLayer3NodebFinalize
// LAYER      :: Layer 3
// PURPOSE    :: Print NodeB specific stats and clear protocol variables.
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + umtsL3    : UmtsLayer3Data * : Pointer to UMTS Layer3 data
// RETURN     :: void : NULL
// **/
void UmtsLayer3NodebFinalize(Node *node, UmtsLayer3Data *umtsL3);
#endif /* _LAYER3_UMTS_NODEB_H_ */
