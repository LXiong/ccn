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

#ifndef _LAYER2_UMTS_MAC_PHY_H
#define _LAYER2_UMTS_MAC_PHY_H

#include <utility>
#include <deque>
#include <list>
#include <deque>
#include <vector>
#include <set>

#include "cellular.h"
#include "cellular_umts.h"

// In this file, it will implement the functions related
// CCrCh, Spreading, Scambling, combining and 
// others not related to RX and RX
// as well as Ch start/stop listening

// /** 
// CONSTANT    :: UMTS_DELAY_UNTIL_SIGNAL_AIRBORN
// DESCRIPTION :: A delay to support look ahead in parallel
#define UMTS_DELAY_UNTIL_SIGNAL_AIRBORN 100 * NANO_SECOND

// /**
// FUNCTION   :: UmtsMacPhyStartListeningToChannel
// LAYER      :: UMTS PHY
// PURPOSE    :: Start listening to a channel
// PARAMETERS ::
// + node : Node* : Pointer to node
// + phyNumber : int : PHY number
// + channelIndex : int : channel to listen to
// RETURN     :: void : NULL
// **/
void UmtsMacPhyStartListeningToChannel(Node* node,
                                       int phyNumber,
                                       int channelIndex);

// /**
// FUNCTION   :: UmtsMacPhyStopListeningToChannel
// LAYER      :: PHY Layer
// PURPOSE    :: Stop listening to a channel
// PARAMETERS ::
// + node : Node* : Pointer to node
// + phyNumber : int : PHY number of the interface
// + channelIndex : int : channel to stop listening to
// RETURN     :: void : NULL
// **/
void UmtsMacPhyStopListeningToChannel(Node* node,
                                      int phyNumber,
                                      int channelIndex);

// /**
// FUNCTION   :: UmtsMacReceivePacketFromPhy
// LAYER      :: MAC
// PURPOSE    :: Receive a packet from physical layer.
//               The PHY sublayer will first handle it
// PARAMETERS ::
// + node             : Node*        : Pointer to node.
// + interfaceIndex   : int          : Interface Index
// + msg              : Message*     : Packet received from PHY
// RETURN     :: void : NULL
// **/
void UmtsMacReceivePacketFromPhy(Node* node,
                                 int interfaceIndex,
                                 Message* msg);

// /**
// FUNCTION   :: UmtsMacUeTransmitMacBurst
// LAYER      :: UMTS LAYER2 MAC sublayer at UE
// PURPOSE    :: Handle timers out messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : interface Index
// + mac              : UmtsMacData*      : Mac Data
// + msg              : Message*          : msg List to send to phy
// RETURN     :: void : NULL
// **/ 
void UmtsMacUeTransmitMacBurst(Node* node,
                               UInt32 interfaceIndex,
                               UmtsMacData* mac,
                               Message* msg);

// /**
// FUNCTION   :: UmtsMacNodeBTransmitMacBurst
// LAYER      :: UMTS LAYER2 MAC sublayer at UE
// PURPOSE    :: Handle timers out messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : interface Index
// + mac              : UmtsMacData*      : Mac Data
// + msg              : Message*          : msg List to send to phy
// RETURN     :: void : NULL
// **/ 
void UmtsMacNodeBTransmitMacBurst(Node* node,
                                  UInt32 interfaceIndex,
                                  UmtsMacData* mac,
                                  Message* msg);

// /** 
// FUNCITON   :: UmtsMacPhyHandleInterLayerCommand
// LAYER      :: UMTS L2 MAC
// PURPOSE    :: Handle Interlayer command CPHY- 
// PARAMETERS :: 
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : interface Index
// + cmdType          : UmtsInterlayerCommandType : command type
// + cmd              : void*          : cmd to handle
// RETURN     :: void : NULL
void UmtsMacPhyHandleInterLayerCommand(
            Node* node,
            UInt32 interfaceIndex,
            UmtsInterlayerCommandType cmdType,
            void* cmd);

// /** 
// FUNCITON   :: UmtsMacPhyMappingTrChPhCh
// LAYER      :: UMTS L2 MAC
// PURPOSE    :: Handle Interlayer command CPHY- 
// PARAMETERS :: 
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : UInt32            : interface Index
// + mappingInfo      : UmtsTrCh2PhChMappingInfo* : Mapping Info
// RETURN     :: void : NULL
void UmtsMacPhyMappingTrChPhCh(
            Node* node,
            UInt32 interfaceIndex,
            UmtsTrCh2PhChMappingInfo* mappingInfo);

// /** 
// FUNCITON   :: UmtsMacPhyConfigRcvPhCh
// LAYER      :: UMTS L2 MAC
// PURPOSE    :: Config receiving side DPDCH information 
//               (channelization code)
// PARAMETERS :: 
// + node             : Node*                   : Pointer to node.
// + interfaceIndex   : UInt32                  : interface Index
// + priSc            : UInt32                  : primary scrambling code
// + phChInfo         : const UmtsRcvPhChInfo*  : receiving physical 
//                                                channel information
// RETURN     :: void : NULL
void UmtsMacPhyConfigRcvPhCh(
        Node* node,
        UInt32 interfaceIndex,
        UInt32 priSc,
        const UmtsRcvPhChInfo* phChInfo);

//  /** 
// FUNCITON   :: UmtsMacPhyReleaseRcvPhCh
// LAYER      :: UMTS L2 MAC
// PURPOSE    :: Release receiving side DPDCH information 
//               (channelization code)
// PARAMETERS :: 
// + node             : Node*                   : Pointer to node.
// + interfaceIndex   : UInt32                  : interface Index
// + priSc            : UInt32                  : primary scrambling code
// + phChInfo         : const UmtsRcvPhChInfo*  : receiving physical 
//                                                channel information
// RETURN     :: void : NULL
void UmtsMacPhyReleaseRcvPhCh(
    Node* node,
    UInt32 interfaceIndex,
    UInt32 priSc,
    const UmtsRcvPhChInfo* phChInfo = NULL);

// /**
// FUNCTION   :: UmtsMacPhyNodeBEnqueuePacketToPhChTxBuffer
// LAYER      :: UMTS L2 MAC-PHY sublayer
// PURPOSE    :: enqueue packet to PhCh txBuffer
// PARAMETERS ::
// node        : Node*                     : Pointer to the node
// interfaceIndex : UInt32                 : interface Index
// chType      : UmtsPhysicalChannelType   : physical channel type
// chId        : unsigned int              : channel Idetifier
// pduList     : std::list<Message*>&      : pointer to msg List
//                                           to be enqueued
// RETURN     :: void : NULL
// **/
void UmtsMacPhyNodeBEnqueuePacketToPhChTxBuffer(
                     Node* node,
                     UInt32 interfaceIndex,
                     UmtsPhysicalChannelType chType,
                     unsigned int chId,
                     std::list<Message*>& pduList);

// /**
// FUNCTION   :: UmtsMacPhyUeEnqueuePacketToPhChTxBuffer
// LAYER      :: UMTS L2 MAC-PHY sublayer
// PURPOSE    :: enqueue packet to PhCh txBuffer
// PARAMETERS ::
// node        : Node*                     : Pointer to the node
// interfaceIndex : UInt32                 : interface Index
// chType      : UmtsPhysicalChannelType   : physical channel type
// chId        : unsigned int              : channel Idetifier
// pduList     : std::list<Message*>&      : pointer to msg List
//                                           to be enqueued
// RETURN     :: void : NULL
// **/
void UmtsMacPhyUeEnqueuePacketToPhChTxBuffer(
                     Node* node,
                     UInt32 interfaceIndex,
                     UmtsPhysicalChannelType chType,
                     unsigned int chId,
                     std::list<Message*>& pduList);

// /**
// FUNCTION   :: UmtsMacPhyGetPccpchSfn
// LAYER      :: Layer 2
// PURPOSE    :: return the PCCPCH SFN at nodeB
// PARAMETERS ::
// + node      : Node * : Pointer to node.
// + interfaceIndex : int : interface index
// + priSc     : unsigned int : primary sc code of the nodeB
// RETURN     :: unsigned int   : PCCPCH SFN index of the nodeB
// **/
unsigned int UmtsMacPhyGetPccpchSfn(Node*node, 
                                     int interfaceIndex,
                                     unsigned int priSc = 0);

// /**
// FUNCTION   :: UmtsMacPhySetPccpchSfn
// LAYER      :: Layer 2
// PURPOSE    :: return the active ul channel index 
//               of the interface at UE/NodeB
// PARAMETERS ::
// + node      : Node * : Pointer to node.
// + interfaceIndex : int : interface index
// + pccpchSfn : unsigned int : PCCPCH FSN
// + priSc     : unsigned int : primary sc code of the nodeB
// RETURN     :: void         : NULL
// **/
void UmtsMacPhySetPccpchSfn(Node*node, 
                             int interfaceIndex, 
                             unsigned int pccpchSfn,
                             unsigned int priSc = 0);

// /**
// FUNCTION   :: UmtsMacPhyNodeBAddUeInfo
// LAYER      :: Layer 2
// PURPOSE    :: return the active ul channel index 
//               of the interface at UE/NodeB
// PARAMETERS ::
// + node      : Node * : Pointer to node.
// + interfaceIndex : int : interface index
// + ueId      : NodeAddress : UE Node Id
// + priSc     : unsigned int : primary sc code of the nodeB
// + selfPrimCell : BOOL  : Is it a priCell for UE
// RETURN     :: void   : NULL
// **/
void UmtsMacPhyNodeBAddUeInfo(Node*node, 
                              int interfaceIndex, 
                              NodeAddress ueId,
                              unsigned int priSc,
                              BOOL selfPrimCell);

// /**
// FUNCTION   :: UmtsMacPhyNodeBRemoveUeInfo
// LAYER      :: Layer 2
// PURPOSE    :: return the active ul channel index 
//               of the interface at UE/NodeB
// PARAMETERS ::
// + node      : Node * : Pointer to node.
// + interfaceIndex : int : interface index
// + ueId      : NodeAddress : UE Node Id
// + priSc     : unsigned int : primary sc code of the nodeB
// RETURN     :: void   : NULL
// **/
void UmtsMacPhyNodeBRemoveUeInfo(Node*node, 
                                 int interfaceIndex, 
                                 NodeAddress ueId,
                                 unsigned int priSc);

// /**
// FUNCTION   :: UmtsMacPhyUeCheckUlPowerControlAdjustment
// LAYER      :: Layer 2
// PURPOSE    :: return the active ul channel index 
//               of the interface at UE/NodeB
// PARAMETERS ::
// + node           : Node *          : Pointer to node.
// + interfaceIndex : int             : interface index
// RETURN     :: void : NULL
// **/
void UmtsMacPhyUeCheckUlPowerControlAdjustment(Node* node,
                                               UInt32 interfaceIndex);

// /**
// FUNCTION   :: UmtsMacPhyNodeBEnableSelfPrimCell
// LAYER      :: Layer 2
// PURPOSE    :: Enable self as the prim cell for this UE
// PARAMETERS ::
// + node      : Node * : Pointer to node.
// + interfaceIndex : int : interface index
// + priSc     : unsigned int : Ue Primary Scrambling code
// RETURN     :: void : NULL
// **/
void UmtsMacPhyNodeBEnableSelfPrimCell(
            Node*node, 
            int interfaceIndex, 
            unsigned int priSc);

// /**
// FUNCTION   :: UmtsMacPhyNodeBDisableSelfPrimCell
// LAYER      :: Layer 2
// PURPOSE    :: Enable self as the prim cell for this UE
// PARAMETERS ::
// + node      : Node * : Pointer to node.
// + interfaceIndex : int : interface index
// + priSc     : unsigned int : Ue Primary Scrambling code
// RETURN     :: void   : NULL
// **/
void UmtsMacPhyNodeBDisableSelfPrimCell(
            Node*node, 
            int interfaceIndex, 
            unsigned int priSc);

// /**
// FUNCTION   :: UmtsMacPhyUeRelAllDedctPhCh
// LAYER      :: Layer 2
// PURPOSE    :: release all  dedicated physical channel
// PARAMETERS ::
// + node           : Node *          : Pointer to node.
// + interfaceIndex : int             : interface index
// RETURN     :: void : NULL
// **/
void UmtsMacPhyUeRelAllDedctPhCh(Node* node,
                                 UInt32 interfaceIndex);

// /**
// FUNCTION   :: UmtsMacPhyUeReleaseAllDch
// LAYER      :: Layer 2
// PURPOSE    :: release all  dedicated Dedicated Transport channels
// PARAMETERS ::
// + node           : Node *          : Pointer to node.
// + interfaceIndex : int             : interface index
// RETURN     :: void : NULL
// **/
void UmtsMacPhyUeRelAllDch(Node* node,
                           UInt32 interfaceIndex);

// /**
// FUNCTION   :: UmtsMacPhyUeSetDataBitSizeInBuffer
// LAYER      :: UMTS L2 MAC-PHY sublayer
// PURPOSE    :: Set the number of data bit in BUffer
// PARAMETERS ::
// node        : Node*                     : Pointer to the node
// interfaceIndex : UInt32                 : interface Index
// numDataBitInBuffer : int                : number of data bit in buffer       
// RETURN     :: void : NULL
// **/
void UmtsMacPhyUeSetDataBitSizeInBuffer(
                     Node* node,
                     UInt32 interfaceIndex,
                     int numDataBitInBuffer);

// /**
// FUNCTION   :: UmtsMacPhyUeGetDataBitSizeInBuffer
// LAYER      :: UMTS L2 MAC-PHY sublayer
// PURPOSE    :: Get the number of data bit in BUffer
// PARAMETERS ::
// node        : Node*                     : Pointer to the node
// interfaceIndex : UInt32                 : interface Index
// RETURN     :: int : numDataBitInBuffer
// **/
int UmtsMacPhyUeGetDataBitSizeInBuffer(
                     Node* node,
                     UInt32 interfaceIndex);

// /**
// FUNCTION   :: UmtsMacPhyUeGetDpdchNumber
// LAYER      :: UMTS L2 MAC-PHY sublayer
// PURPOSE    :: Get the number of DPDCH channels
// PARAMETERS ::
// node        : Node*                     : Pointer to the node
// interfaceIndex : UInt32                 : interface Index
// RETURN     :: unsigned char : number of dedicated DPDCH
// **/
unsigned char UmtsMacPhyUeGetDpdchNumber(
                     Node* node,
                     UInt32 interfaceIndex);

// /**
// FUNCTION   :: UmtsMacPhyUeSetDpdchNumber
// LAYER      :: UMTS L2 MAC-PHY sublayer
// PURPOSE    :: Get the number of data bit in BUffer
// PARAMETERS ::
// node        : Node*                     : Pointer to the node
// interfaceIndex : UInt32                 : interface Index
// numUlDpdch  : unsigned char             : umber of dedicated DPDCH       
// RETURN     :: void : NULL
// **/
void UmtsMacPhyUeSetDpdchNumber(
                     Node* node,
                     UInt32 interfaceIndex,
                     unsigned char numUlDpdch);

// /**
// FUNCTION   :: UmtsMacPhyUeSetHsdpaCqiReportInd
// LAYER      :: UMTS L2 MAC-PHY sublayer
// PURPOSE    :: Set the hsdpa Cqi report request indicator
// PARAMETERS ::
// node        : Node*                     : Pointer to the node
// interfaceIndex : UInt32                 : interface Index
// RETURN     :: void : NULL
// **/
void UmtsMacPhyUeSetHsdpaCqiReportInd(
                     Node* node,
                     UInt32 interfaceIndex); 

// /**
// FUNCTION   :: UmtsMacPhyNodeBSetHsdpaConfig
// LAYER      :: UMTS L2 MAC-PHY sublayer
// PURPOSE    :: Set the hsdpa configuration
// PARAMETERS ::
// node        : Node*                     : Pointer to the node
// interfaceIndex : UInt32                 : interface Index
// numHspdsch  : unsigned char             : numer of HS-PDSCH
// hspdschId   : unsigned int*             : channel Id
// numHsscch   : unsigned char             : number of HS-SCCH
// hsscchId    : unsigned int*             : channel Id
// RETURN     :: void : NULL
// **/ 
void UmtsMacPhyNodeBSetHsdpaConfig(
                     Node* node,
                     UInt32 interfaceIndex,
                     unsigned char numHspdsch,
                     unsigned int* hspdschId,
                     unsigned char numHsscch,
                     unsigned int* hsscchId);

// /**
// FUNCTION   :: UmtsMacPhyNodeBGetHsdpaConfig
// LAYER      :: UMTS L2 MAC-PHY sublayer
// PURPOSE    :: Get the hsdpa configuration
// PARAMETERS ::
// node        : Node*                     : Pointer to the node
// interfaceIndex : UInt32                 : interface Index
// numHspdsch  : unsigned char*             : numer of HS-PDSCH
// hspdschId   : unsigned int*             : channel Id
// numHsscch   : unsigned char*             : number of HS-SCCH
// hsscchId    : unsigned int*             : channel Id
// RETURN     :: void : NULL
// **/ 
void UmtsMacPhyNodeBGetHsdpaConfig(
                     Node* node,
                     UInt32 interfaceIndex,
                     unsigned char* numHspdsch,
                     unsigned int* hspdschId,
                     unsigned char* numHsscch,
                     unsigned int* hsscchId);

// /**
// FUNCTION   :: UmtsMacPhyNodeBGetActiveHsdpaChNum
// LAYER      :: Layer 2
// PURPOSE    :: return the active HSDPA channels
// PARAMETERS ::
// + node      : Node * : Pointer to node.
// + interfaceIndex : int : interface index
// + numActiveCh : unsigned int* : Number of active HSDPA Channels
// RETURN     :: void : NULL
// **/
void UmtsMacPhyNodeBGetActiveHsdpaChNum(Node*node,
                                        UInt32 interfaceIndex,
                                        unsigned int* numActiveCh);

// /**
// FUNCTION   :: UmtsMacPhyNodeBSetActiveHsdpaChNum
// LAYER      :: Layer 2
// PURPOSE    :: set the active HSDPA channels
// PARAMETERS ::
// + node      : Node * : Pointer to node.
// + interfaceIndex : int : interface index
// + numActiveCh : unsigned int : Number of active HSDPA Channels
// RETURN     :: void : NULL
// **/
void UmtsMacPhyNodeBSetActiveHsdpaChNum(Node*node,
                                        UInt32 interfaceIndex,
                                        unsigned int numActiveCh);

// /**
// FUNCTION   :: UmtsMacPhyNodeBGetUeCqiInfo
// LAYER      :: Layer 2
// PURPOSE    :: return the CQI value at UE/NodeB
// PARAMETERS ::
// + node      : Node * : Pointer to node.
// + interfaceIndex : int : interface index
// + priSc : unsigned int : primry sc code
// + cqiVal : unsigned chr* : cqivl reproted from UE
// RETURN     :: void : NULL
// **/
void UmtsMacPhyNodeBGetUeCqiInfo(Node*node,
                                 UInt32 interfaceIndex,
                                 unsigned int priSc,
                                 unsigned char* cqiVal);

// /**
// FUNCTION   :: UmtsMacPhyNodeBUpdatePhChInfo
// LAYER      :: Layer 2
// PURPOSE    :: Update Physical channel info
// PARAMETERS ::
// + node      : Node * : Pointer to node.
// + interfaceIndex : int : interface index
// + trChType    : UmtsPhysicalChannelType   : physical channel type
// + chId        : unsigned int              : channel Identifier
// + upInfo      : UmtsPhChUpdateInfo*       : updated info
// RETURN     :: void : NULL
// **/ 
BOOL UmtsMacPhyNodeBUpdatePhChInfo(
                     Node* node,
                     UInt32 interfaceIndex,
                     UmtsPhysicalChannelType chType,
                     unsigned int chId,
                     UmtsPhChUpdateInfo* upInfo);

// /**
// FUNCTION   :: UmtsMacPhyGetSignalBitErrorRate
// LAYER      :: Layer 2
// PURPOSE    :: return the CQI value at UE/NodeB
// PARAMETERS ::
// + node      : Node * : Pointer to node.
// + interfaceIndex : int : interface index
// + codingType : UmtsCodingType  : Coding type 
// + moduType   : UmtsModulationType : Modulation type
// + sinr       : double : sinr
// RETURN     :: double : BER
// **/
double UmtsMacPhyGetSignalBitErrorRate(
                     Node* node,
                     UInt32 interfaceIndex,
                     UmtsCodingType codingType,
                     UmtsModulationType moduType,
                     double sinr);
#endif //_LAYER2_UMTS_MAC_PHY_H



