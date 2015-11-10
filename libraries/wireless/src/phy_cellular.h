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

// /**
// PACKAGE     :: PHY_CELLULAR
// DESCRIPTION :: Defines the data structures used in the CELLULAR Model
//                Most of the time the data structures and functions start
//                with CellularPhy**
// **/

#ifndef _PHY_CELLULAR_H_
#define _PHY_CELLULAR_H_

#include "cellular.h"

#ifdef UMTS_LIB
//UMTS
#include "cellular_umts.h"
#include "phy_umts.h"
#endif

#ifdef MUOS_LIB
//MUOS
#include "cellular_muos.h"
#include "phy_muos.h"
#endif

// /**
// ENUM        :: CellularPhyProtocolType
// DESCRIPTION :: Enlisted different Cellular Protocols
// **/
typedef enum
{
    Cellular_ABSTRACT_Phy = 0,
    Cellular_GSM_Phy,
    Cellular_GPRS_Phy,
    Cellular_UMTS_Phy,
    Cellular_CDMA2000_Phy,
    Cellular_WIMAX_Phy,
    Cellular_MUOS_Phy,
}CellularPhyProtocolType;

// /**
// STRUCT      :: PhyCellularData
// DESCRIPTION :: Cellular PHY Data
// **/
typedef struct
{
    // Must be set at initialization for every Cellular node
    PhyData* thisPhy;
    RandomSeed randSeed;
    CellularNodeType nodeType;
    CellularPhyProtocolType cellularPhyProtocolType;

#ifdef UMTS_LIB
    // UMTS
    PhyUmtsData* phyUmtsData;
#endif

#ifdef MUOS_LIB
    // MUOS
    PhyMuosData* phyMuosData;
#endif

    BOOL collectStatistics;
} PhyCellularData;

//--------------------------------------------------------------------------
//  Key API functions of the CELLULAR PHY
//--------------------------------------------------------------------------

// /**
// FUNCTION   :: PhyCellularInit
// LAYER      :: PHY
// PURPOSE    :: Initialize the Cellular PHY
// PARAMETERS ::
// + node      : Node*            : Pointer to node.
// + phyIndex  : const int        : Index of the PHY
// + nodeInput : const NodeInput* : Pointer to the node input
// RETURN     :: void             : NULL
// **/
void PhyCellularInit(Node *node,
                     const int phyIndex,
                     const NodeInput *nodeInput);

// /**
// FUNCTION   :: PhyCellularFinalize
// LAYER      :: PHY
// PURPOSE    :: Finalize the CELLULAR PHY, print out statistics
// PARAMETERS ::
// + node      : Node*     : Pointer to node.
// + phyIndex  : const int : Index of the PHY
// RETURN     :: void      : NULL
// **/
void PhyCellularFinalize(Node *node, const int phyIndex);

// /**
// FUNCTION   :: PhyCellularStartTransmittingSignal
// LAYER      :: PHY
// PURPOSE    :: Start transmitting a frame
// PARAMETERS ::
// + node              : Node* : Pointer to node.
// + phyIndex          : int   : Index of the PHY
// + packet : Message* : Frame to be transmitted
// + clocktype : duration : Duration of the transmission
// + useMacLayerSpecifiedDelay : Use MAC specified delay or calculate it
// + initDelayUntilAirborne    : clocktype : The MAC specified delay
// RETURN     :: void : NULL
// **/
void PhyCellularStartTransmittingSignal(
         Node* node,
         int phyIndex,
         Message* packet,
         clocktype duration,
         BOOL useMacLayerSpecifiedDelay,
         clocktype initDelayUntilAirborne);

// /**
// FUNCTION   :: PhyCellularTransmissionEnd
// LAYER      :: PHY
// PURPOSE    :: End of the transmission
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// RETURN     :: void  : NULL
// **/
void PhyCellularTransmissionEnd(
         Node* node,
         int phyIndex);

// /**
// FUNCTION   :: PhyCellularSignalArrivalFromChannel
// LAYER      :: PHY
// PURPOSE    :: Handle signal arrival from the chanel
// PARAMETERS ::
// + node         : Node* : Pointer to node.
// + phyIndex     : int   : Index of the PHY
// + channelIndex : int   : Index of the channel receiving signal from
// + propRxInfo   : PropRxInfo* : Propagation information
// RETURN     :: void : NULL
// **/
void PhyCellularSignalArrivalFromChannel(
         Node* node,
         int phyIndex,
         int channelIndex,
         PropRxInfo* propRxInfo);

// /**
// FUNCTION   :: PhyCellularSignalEndFromChannel
// LAYER      :: PHY
// PURPOSE    :: Handle signal end from a chanel
// PARAMETERS ::
// + node         : Node* : Pointer to node.
// + phyIndex     : int   : Index of the PHY
// + channelIndex : int   : Index of the channel receiving signal from
// + propRxInfo   : PropRxInfo* : Propagation information
// RETURN     :: void : NULL
// **/
void PhyCellularSignalEndFromChannel(
         Node* node,
         int phyIndex,
         int channelIndex,
         PropRxInfo* propRxInfo);

// /**
// FUNCTION   :: PhyCellularGetStatus
// LAYER      :: PHY
// PURPOSE    :: Get the status of the PHY.
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + phyNum    : int           : Index of the PHY
// RETURN     :: PhyStatusType : Status of the PHY
// **/
PhyStatusType
PhyCellularGetStatus(
    Node* node,
    int phyNum);

// /**
// FUNCTION   :: PhyDot16SetTransmitPower
// LAYER      :: PHY
// PURPOSE    :: Set the transmit power of the PHY
// PARAMETERS ::
// + node       : Node*  : Pointer to node.
// + phyIndex   : int    : Index of the PHY
// + txPower_mW : double : Transmit power in mW unit
// RETURN     ::  void   : NULL
// **/
void PhyCellularSetTransmitPower(
         Node* node,
         PhyData* thisPhy,
         double newTxPower_mW);

// /**
// FUNCTION   :: PhyDot16GetTransmitPower
// LAYER      :: PHY
// PURPOSE    :: Get the transmit power of the PHY
// PARAMETERS ::
// + node       : Node*   : Pointer to node.
// + phyIndex   : int     : Index of the PHY
// + txPower_mW : double* : Transmit power in mW unit to be return
// RETURN     ::  void   : NULL
// **/
void PhyCellularGetTransmitPower(
         Node* node,
         PhyData* thisPhy,
         double* txPower_mW);

// /**
// FUNCTION         :: PhyCellularGetDataRate
// LAYER            :: PHY
// PURPOSE          :: Get the raw data rate
// PARAMETERS       ::
// + node            : Node*       : Pointer to node.
// + thisPhy         : PhyData*    : Pointer to PHY structure
// RETURN           :: int      : Data rate in bps
// **/
int PhyCellularGetDataRate(
        Node* node,
        PhyData* thisPhy);
// /**
// FUNCTION         :: PhyCellularGetRxLevel
// LAYER            :: PHY
// PURPOSE          :: Get the Rx level
// PARAMETERS       ::
// + node            : Node*       : Pointer to node.
// + thisPhy         : PhyData*    : Pointer to PHY structure
// RETURN           :: float       : Rx level
// **/
float PhyCellularGetRxLevel(
          Node* node,
          int phyIndex);

// /**
// FUNCTION         :: PhyCellularGetRxQuality
// LAYER            :: PHY
// PURPOSE          :: Get the raw data rate
// PARAMETERS       ::
// + node            : Node*       : Pointer to node.
// + thisPhy         : PhyData*    : Pointer to PHY structure
// RETURN           :: double      : Data rate in bps
// **/
double PhyCellularGetRxQuality(
           Node* node,
           int phyIndex);

// /**
// FUNCTION   :: PhyCellularInterferenceArrivalFromChannel
// LAYER      :: PHY
// PURPOSE    :: Handle signal arrival from the chanel
// PARAMETERS ::
// + node         : Node* : Pointer to node.
// + phyIndex     : int   : Index of the PHY
// + channelIndex : int   : Index of the channel receiving signal from
// + propRxInfo   : PropRxInfo* : Propagation information
// + sigPower_mW  : double      : The inband interference signal power
// RETURN     :: void : NULL
// **/
void PhyCellularInterferenceArrivalFromChannel(
         Node* node,
         int phyIndex,
         int channelIndex,
         PropRxInfo* propRxInfo,
         double sigPower_mW);


 // /**
// FUNCTION   :: PhyCellularInterferenceEndFromChannel
// LAYER      :: PHY
// PURPOSE    :: Handle signal end from a chanel
// PARAMETERS ::
// + node         : Node* : Pointer to node.
// + phyIndex     : int   : Index of the PHY
// + channelIndex : int   : Index of the channel receiving signal from
// + propRxInfo   : PropRxInfo* : Propagation information
// RETURN     :: void : NULL
// **/
void PhyCellularInterferenceEndFromChannel(
         Node* node,
         int phyIndex,
         int channelIndex,
         PropRxInfo* propRxInfo);

#endif /* _PHY_CELLULAR_H_ */
