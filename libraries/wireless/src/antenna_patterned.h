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

#ifndef ANTENNA_PATTERNED_H
#define ANTENNA_PATTERNED_H

#ifdef __cplusplus
extern "C" {
#endif

#include "antenna_global.h"

// /**
// CONSTANT :: PATTERNED_AZIMUTH_INDEX : 0
// DESCRIPTION :: Const for azimuth index of Patterned antenna
// **/

#define PATTERNED_AZIMUTH_INDEX 0


// /**
// CONSTANT :: PATTERNED_ELEVATION_INDEX : 1                     :   1
// DESCRIPTION :: Const for elevation index of Patterned antenna
// **/

#define PATTERNED_ELEVATION_INDEX 1


// /**
// STRUCT :: struct_Antenna_Patterned
// DESCRIPTION :: Structure for Patterned antenna
// **/

typedef struct struct_Antenna_Patterned {
    int                     modelIndex;
    int                     numPatterns;
    int                     patternIndex;
    float             antennaHeight;
    float             antennaGain_dB;
    AntennaPattern          *pattern;
} AntennaPatterned;

// /**
// FUNCTION :: ANTENNA_PatternedInitFromConfigFile
// LAYER :: PHYSICAL
// PURPOSE :: Init the Patterned antennas from
//            configuration file.
// PARAMETERS ::
// + node : Node* : Node pointer that the antenna is being
//                  instantiated in
// + phyIndex : int : interface for which physical
//                    to be initialized
// + nodeInput : const NodeInput* : Pointer to NodeInput structure
// RETURN :: void : NULL
// **/

void ANTENNA_PatternedInitFromConfigFile(
     Node* node,
     int phyIndex,
     const NodeInput* nodeInput);

// /**
// FUNCTION :: ANTENNA_PatternedInit
// LAYER :: Physical Layer.
// PURPOSE :: initialization of patterned antenna.
// PARAMETERS ::
// + node : Node* : Pointer to Node.
// + nodeInput : const NodeInput* : Pointer to NodeInput structure
// + phyIndex : int : interface for which physical to be initialized
// + antennaModel : const AntennaModelGlobal* : pointer to global
//                                              antenna model
// RETURN :: void : NULL
// **/

void ANTENNA_PatternedInit(
     Node* node,
     const NodeInput* nodeInput,
     int phyIndex,
     const AntennaModelGlobal* antennaModel);


// /**
// FUNCTION  :: AntennaPatternedUsingDefaultPattern
// LAYER :: Physical Layer.
// PURPOSE :: Is using default pattern.
// PARAMETERS ::
// + phyData : PhyData* : pointer to the struct PhyData
// RETURN :: BOOL : Return TRUE if antenna pattern index
//                  is ANTENNA_DEFAULT_PATTERN
// **/

BOOL AntennaPatternedUsingDefaultPattern(
     PhyData* phyData);


// /**
// FUNCTION :: AntennaPatternedSetPattern
// LAYER :: Physical Layer.
// PURPOSE :: Set the current pattern.
// PARAMETERS ::
// + node : Node* : node being initialized.
// + phyIndex : int : interface for which physical to be
//                    initialized.
// + patternIndex : int : index of the pattern which is set
// RETURN :: void : NULL
// **/

void AntennaPatternedSetPattern(
     Node *node,
     int phyIndex,
     int patternIndex);


// /**
// FUNCTION :: AntennaPatternedGetPattern
// LAYER :: Physical Layer.
// PURPOSE :: Set the current pattern.
// PARAMETERS ::
// + node : Node* : node being initialized.
// + phyIndex : int : interface for which physical to be
//                    initialized.
// + patternIndex : int* : index of the pattern which is set
// RETURN :: void : NULL
// **/

void AntennaPatternedGetPattern(
     Node *node,
     int phyIndex,
     int *patternIndex);


// /**
// FUNCTION :: AntennaPatternedReturnPatternIndex
// LAYER :: Physical Layer.
// PURPOSE :: Return the current pattern Index.
// PARAMETERS ::
// + node : Node* : node being initialized.
// + phyIndex : int : interface for which physical to be
//                    initialized.
// RETURN :: int : Return the patternIndex
// **/

int AntennaPatternedReturnPatternIndex(
    Node* node,
    int phyIndex);


// /**
// FUNCTION :: AntennaPatternedGainForThisDirection
// LAYER :: Physical Layer.
// PURPOSE :: Return gain for this direction.
// PARAMETERS ::
// + node : Node* : node being initialized.
// + phyIndex : int : interface for which physical to be
//                    initialized.
// + DOA : Orientation : dirrection of arrival of current
//                       signal
// RETURN :: float : Return the antennaGain
// **/

float AntennaPatternedGainForThisDirection(
      Node *node,
      int phyIndex,
      Orientation DOA);


// /**
// FUNCTION :: AntennaPatternedGainForThisDirectionWithPatternIndex
// LAYER :: Physical Layer.
// PURPOSE :: Return gain for current pattern index.
// PARAMETERS ::
// + node : Node* : node being initialized.
// + phyIndex : int : interface for which physical to be
//                    initialized.
// + patternIndex : int : index of the pattern which is set
// + DOA : Orientation : dirrection of arrival of current
//                       signal
// RETURN :: float : Return the antennaGain
// **/

float AntennaPatternedGainForThisDirectionWithPatternIndex(
      Node *node,
      int phyIndex,
      int patternIndex,
      Orientation DOA);


// /**
// FUNCTION :: AntennaPatternedMaxGainPatternForThisSignal
// LAYER :: Physical Layer.
// PURPOSE :: Return gain for pattern index.
// PARAMETERS ::
// + node : Node* : node being initialized.
// + phyIndex : int : interface for which physical to be
//                    initialized.
// + propRxInfo : PropRxInfo* : pointer to PropRxInfo
// + patternIndex : int* : index of the pattern
// + gain_dBi : float* : contains the gain value.
// RETURN :: void : NULL
// **/

void AntennaPatternedMaxGainPatternForThisSignal(
    Node* node,
    int phyIndex,
    PropRxInfo* propRxInfo,
    int* patternIndex,
    float* gain_dBi);


// /**
// FUNCTION :: AntennaPatternedSetBestPatternForAzimuth
// LAYER :: Physical Layer.
// PURPOSE :: Set best pattern for azimith.
// PARAMETERS ::
// + node : Node* : node being initialized.
// + phyIndex : int : interface for which physical to be
//                    initialized.
// + azimuthAngle : double : azimuth angle
// RETURN :: void : NULL
// **/

void AntennaPatternedSetBestPatternForAzimuth(
     Node *node,
     int phyIndex,
     double azimuthAngle);


// /**
// FUNCTION :: ANTENNA_PatternedSetBestPatternForAzimuth
// LAYER :: Physical Layer.
// PURPOSE :: Set best pattern for azimith.
// PARAMETERS ::
// + node : Node* : node being initialized.
// + phyIndex : int : interface for which physical to be
//                    initialized.
// + azimuthAngle : double : azimuth angle
// RETURN :: void : NULL
// **/

void ANTENNA_PatternedSetBestPatternForAzimuth(
     Node* node,
     int phyIndex,
     double azimuthAngle);


// /**
// FUNCTION  :: ANTENNA_PatternedGainForThisDirection
// LAYER :: Physical Layer.
// PURPOSE :: Return gain for a particular direction.
// PARAMETERS ::
// + node : Node* : node being initialized.
// + phyIndex : int : interface for which physical to be
//                    initialized.
// + DOA : Orientation : azimuth & elvation angle
// RETURN :: float : Return antennaGain
// **/

float ANTENNA_PatternedGainForThisDirection(
      Node* node,
      int phyIndex,
      Orientation DOA);


// inlines //

// /**
// FUNCTION  :: AntennaPatternedLockAntennaDirection
// LAYER :: Physical Layer.
// PURPOSE :: Lock antenna dirrection.
// PARAMETERS ::
// + node : Node* : node being initialized.
// + phyIndex : int : interface for which physical to be
//                    initialized.
// RETURN :: void : NULL
// **/

static
void AntennaPatternedLockAntennaDirection(
     Node *node,
     int phyIndex)
{
    PhyData *phyData = node->phyData[phyIndex];
    ERROR_Assert(phyData->antennaData->antennaModelType ==
        ANTENNA_PATTERNED ,
            "antennaModelType not patterned.\n");
}


// /**
// FUNCTION :: AntennaPatternedUnLockAntennaDirection
// LAYER :: Physical Layer.
// PURPOSE :: UnLock antenna dirrection.
// PARAMETERS ::
// + node : Node* : node being initialized.
// + phyIndex : int : interface for which physical to be
//                    initialized.
// RETURN :: void : NULL
// **/

static
void AntennaPatternedUnlockAntennaDirection(
     Node *node,
     int phyIndex)
{
    PhyData *phyData = node->phyData[phyIndex];
    ERROR_Assert(phyData->antennaData->antennaModelType ==
        ANTENNA_PATTERNED ,
            "antennaModelType not patterned.\n");
}


// /**
// FUNCTION :: AntennaPatternedIsLocked
// LAYER :: Physical Layer.
// PURPOSE :: Lock antenna.
// PARAMETERS ::
// + node : Node* : node being initialized.
// + phyIndex : int : interface for which physical to be
//                    initialized.
// RETURN :: BOOL : Return TRUE if antenna is locked
// **/

static
BOOL AntennaPatternedIsLocked(
     Node *node,
     int phyIndex)
{
    PhyData *phyData = node->phyData[phyIndex];
    ERROR_Assert(phyData->antennaData->antennaModelType ==
        ANTENNA_PATTERNED ,
            "antennaModelType not patterned.\n");

    return FALSE;
}

#ifdef __cplusplus
}
#endif

#endif /* ANTENNA_PATTERNED_H */

