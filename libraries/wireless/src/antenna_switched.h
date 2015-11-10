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

#ifndef ANTENNA_SWITCHED_H
#define ANTENNA_SWITCHED_H

#include "antenna.h"

#define SWITCHED_AZIMUTH_INDEX 0
#define SWITCHED_ELEVATION_INDEX 1


// /**
// STRUCT      :: AntennaSwitchedBeam 
// DESCRIPTION :: This structure contains the data pertaining to 
//                Switched beam Antenna. The structure is assigned to 
//                the phydata's (void*) antennaVar
// **/


typedef struct struct_Antenna_Switched_Beam {
    int     patternIndex;
    int     lastPatternIndex;
    float   antennaHeight;
    float   antennaGain_dB;
    BOOL antennaIsLocked;
    int  numPatterns;
    AntennaPattern*  antennaPatterns;
}AntennaSwitchedBeam;


// /**
// FUNCTION :: ANTENNA_SwitchedBeamInit
// LAYER :: PHYSICAL
// PURPOSE :: Init the switchedbeam antennas from
//              antenna-model config file.
// PARAMETERS ::
// + node : Node* : Node pointer that the antenna is being
//                  instantiated in
// + nodeInput : const NodeInput* : Pointer to NodeInput structure
// + phyIndex : int : interface for which physical
//                  to be initialized
// + antennaModel : const AntennaModelGlobal* : pointer to Global
//                                              antenna model structure
// RETURN :: void : NULL
// **/

void ANTENNA_SwitchedBeamInit(
     Node* node,
     const NodeInput* nodeInput,
     int phyIndex,
     const AntennaModelGlobal* antennaModel);


// /**
// FUNCTION :: ANTENNA_SwitchedBeamInitFromConfigFile()
// LAYER :: PHYSICAL
// PURPOSE :: Initialize data structure.
// PARAMETERS ::
// + node : Node* : Node pointer that the antenna is being
//                  instantiated in
// + phyIndex : int : interface for which physical
//                  to be initialized
// + nodeInput : const NodeInput* : Pointer to NodeInput structure
// RETURN :: void : NULL
// **/

void ANTENNA_SwitchedBeamInitFromConfigFile(
     Node* node,
     int phyIndex,
     const NodeInput* nodeInput);


// /**
// FUNCTION :: AntennaSwitchedBeamOmnidirectionalPattern
// LAYER :: PHYSICAL
// PURPOSE :: Is using omnidirectional pattern.
// PARAMETERS ::
// + phyData : PhyData* : Pointer to PhyData structure
// RETURN :: BOOL : Return TRUE if patternIndex is
//                  ANTENNA_OMNIDIRECTIONAL_PATTERN
// **/

BOOL AntennaSwitchedBeamOmnidirectionalPattern(
    PhyData* phyData);


// /**
// FUNCTION :: AntennaSwitchedBeamReturnPatternIndex
// LAYER :: PHYSICAL
// PURPOSE :: Return current pattern index.
// PARAMETERS ::
// + node : Node* : Node pointer that the antenna is being
//                  instantiated in
// + phyIndex : int : interface for which physical
//                    to be initialized
// RETURN :: int : Return the patternIndex
// **/

int AntennaSwitchedBeamReturnPatternIndex(
    Node* node,
    int phyIndex);


// /**
// FUNCTION :: AntennaSwitchedBeamGainForThisDirection
// LAYER :: PHYSICAL
// PURPOSE :: Return gain for this direction.
// PARAMETERS ::
// + node : Node* : Node pointer that the antenna is being
//                  instantiated in
// + phyIndex : int : interface for which physical
//                  to be initialized
// + DOA : Orientation : Direction of arrival
// RETURN :: float : Return the antennaGain
// **/

float AntennaSwitchedBeamGainForThisDirection(
      Node* node,
      int phyIndex,
      Orientation DOA);


// /**
// FUNCTION :: AntennaSwitchedBeamGainForThisDirectionWithPatternIndex
// LAYER :: PHYSICAL
// PURPOSE :: Return gain for current pattern index.
// PARAMETERS ::
// + node : Node* : Node pointer that the antenna is being
//                  instantiated in
// + phyIndex : int : interface for which physical
//                    to be initialized
// + patternIndex : int : Index of a specified antenna pattern
// + DOA : Orientation : Direction of arrival
// RETURN :: float : Return the antennaGain
// **/

float AntennaSwitchedBeamGainForThisDirectionWithPatternIndex(
      Node* node,
      int phyIndex,
      int patternIndex,
      Orientation DOA);


// /**
// FUNCTION :: AntennaSwitchedBeamSetPattern
// LAYER :: PHYSICAL
// PURPOSE :: Set the current pattern.
// PARAMETERS ::
// + node : Node* : Node pointer that the antenna is being
//                  instantiated in
// + phyIndex : int : interface for which physical
//                    to be initialized
// + patternIndex : int : Index of a specified antenna pattern
// RETURN :: void : NULL
// **/

void AntennaSwitchedBeamSetPattern(
     Node* node,
     int phyIndex,
     int patternIndex);


// /**
// FUNCTION :: AntennaSwitchedBeamGetPattern
// LAYER :: PHYSICAL
// PURPOSE :: Get the current pattern.
// PARAMETERS ::
// + node : Node* : Node pointer that the antenna is being
//                  instantiated in
// + phyIndex : int : interface for which physical
//                    to be initialized
// + patternIndex : int* : Index of a specified antenna pattern
// RETURN :: void : NULL
// **/

void AntennaSwitchedBeamGetPattern(
     Node* node,
     int phyIndex,
     int* patternIndex);


// /**
// FUNCTION :: AntennaSwitchedBeamMaxGainPatternForThisSignal
// LAYER :: PHYSICAL
// PURPOSE :: Return heighest gain and pattern index
//            for a signal
// PARAMETERS ::
// + node : Node* : Node pointer that the antenna is being
//                  instantiated in
// + phyIndex : int : interface for which physical
//                  to be initialized
// + propRxInfo : PropRxInfo* : Pointer to PropRxInfo structure
// + patternIndex : int* : Index of a specified antenna pattern
// + gain_dBi : float* : Store antenna gain
// RETURN :: void : NULL
// **/

void AntennaSwitchedBeamMaxGainPatternForThisSignal(
     Node* node,
     int phyIndex,
     PropRxInfo* propRxInfo,
     int* patternIndex,
     float* gain_dBi);

// /**
// FUNCTION :: AntennaSwitchedBeamIsLocked
// LAYER :: PHYSICAL
// PURPOSE :: Returns TRUE is antenna is locked in one direction  
//            and returns false otherwise 
// PARAMETERS ::
// + node : Node* : Node pointer that the antenna is being
//                  instantiated in
// + phyIndex : int : interface for which physical layer is
//                  to be initialized
// RETURN :: BOOL
// **/

static //inline//
BOOL AntennaSwitchedBeamIsLocked(
    Node* node,
    int phyIndex)
{
    PhyData* phyData = node->phyData[phyIndex];
    AntennaSwitchedBeam* switched =
        (AntennaSwitchedBeam* )phyData->antennaData->antennaVar;

    ERROR_Assert(phyData->antennaData->antennaModelType ==
        ANTENNA_SWITCHED_BEAM ,
            "antennaModelType is not switched beam.\n");

    return (switched->antennaIsLocked);
}

// /**
// FUNCTION :: AntennaSwitchedBeamLockAntennaDirection 
// LAYER :: PHYSICAL
// PURPOSE :: Locks the antenna in one direction 
// PARAMETERS ::
// + node : Node* : Node pointer that the antenna is being
//                  instantiated in
// + phyIndex : int : interface for which physical layer is
//                  to be initialized
// RETURN :: void
// **/

static //inline//
void AntennaSwitchedBeamLockAntennaDirection(
    Node* node,
    int phyIndex)
{
    PhyData* phyData = node->phyData[phyIndex];
    AntennaSwitchedBeam* switched =
        (AntennaSwitchedBeam* )phyData->antennaData->antennaVar;

    ERROR_Assert(phyData->antennaData->antennaModelType ==
        ANTENNA_SWITCHED_BEAM ,
            "antennaModelType is not switched beam.\n");

    switched->antennaIsLocked = TRUE;
}


// /**
// FUNCTION :: AntennaSwitchedBeamUnlockAntennaDirection 
// LAYER :: PHYSICAL
// PURPOSE :: UN-Locks the antenna 
// PARAMETERS ::
// + node : Node* : Node pointer that the antenna is being
//                  instantiated in
// + phyIndex : int : interface for which physical layer is
//                  to be initialized
// RETURN :: void
// **/
static //inline//
void AntennaSwitchedBeamUnlockAntennaDirection(
    Node* node,
    int phyIndex)
{
    PhyData* phyData = node->phyData[phyIndex];
    AntennaSwitchedBeam* switched =
        (AntennaSwitchedBeam* )phyData->antennaData->antennaVar;

    ERROR_Assert(phyData->antennaData->antennaModelType ==
        ANTENNA_SWITCHED_BEAM ,
            "antennaModelType is not switched beam.\n");

    switched->antennaIsLocked = FALSE;
}

// /**
// FUNCTION :: AntennaSwitchedBeamSetBestPatternForAzimuth 
// LAYER :: PHYSICAL
// PURPOSE :: Looks through the switched beam patterns and finds the 
//            one closest to the new value of azimuthal angle that is
//            received as an input parameter. Sets the beam to the value
//            found.
// PARAMETERS ::
// + node : Node* : Node pointer that the antenna is being
//                  instantiated in
// + phyIndex : int : interface for which physical layer is
//                  to be initialized
// + azimuthAngle : double : New value of the antenna's azimuthal Angle 
// RETURN :: void
// **/


static //inline//
void AntennaSwitchedBeamSetBestPatternForAzimuth(
    Node* node,
    int phyIndex,
    double azimuthAngle)
{
    PhyData* phyData = node->phyData[phyIndex];
    AntennaSwitchedBeam* switched =
        (AntennaSwitchedBeam* )phyData->antennaData->antennaVar;
    double azimuthDelta = 360.0;
    int i;

    ERROR_Assert(phyData->antennaData->antennaModelType ==
        ANTENNA_SWITCHED_BEAM ,
            "antennaModelType is not switched beam.\n");

    for (i = 0; i < switched->numPatterns; i++) {
        double boreSightAngle =
                    switched->antennaPatterns->boreSightAngle[i].azimuth;

        double delta =
           MIN(fabs(azimuthAngle - boreSightAngle),
               fabs(azimuthAngle - (boreSightAngle - 360.0)));

        if (azimuthDelta > delta) {
            switched->patternIndex = i;
            azimuthDelta = delta;
        }//if//
    }//for//

    //GuiStart
    if (node->guiOption) {
        GUI_SetPatternIndex(node,
                            phyData->macInterfaceIndex,
                            switched->patternIndex,
                            node->getNodeTime());
    }
    //GuiEnd

}

#endif /* ANTENNA_SWITCHED_H */

