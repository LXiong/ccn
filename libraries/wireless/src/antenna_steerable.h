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

#ifndef ANTENNA_STEERABLE_H
#define ANTENNA_STEERABLE_H

#define STEERABLE_AZIMUTH_INDEX 0
#define STEERABLE_ELEVATION_INDEX 1

// /**
// STRUCT      :: AntennaSteerable
// DESCRIPTION :: This structure contains the data pertaining to 
//                Steerable Antenna. The structure is assigned to 
//                the phydata's (void*) antennaVar
// **/


typedef struct struct_Antenna_Steerable {
    int     patternSetRepeatAngle;
    int     patternIndex;
    float   antennaHeight;
    float   antennaGain_dB;            // Max antenna gain
    Orientation steeringAngle;
    BOOL antennaIsLocked;
    int  numPatterns;
    AntennaPattern* antennaPatterns;
} AntennaSteerable;



// /**
// FUNCTION :: ANTENNA_SteerableInit
// LAYER :: PHYSICAL
// PURPOSE :: Init the steerable antennas from
//            antenna-model config file.
// PARAMETERS
// + node : Node* : Node pointer that the antenna is being
//                  instantiated in
// + nodeInput : const NodeInput* : Pointer to NodeInput structure
// + phyIndex : int : interface for which physical layer is
//                  to be initialized
// + antennaModel : const AntennaModelGlobal* : pointer to Global
//                                              antenna model structure
// RETURN :: void : NULL
// **/

void ANTENNA_SteerableInit(
    Node* node,
    const NodeInput* nodeInput,
    int phyIndex,
    const AntennaModelGlobal* antennaModel);


// /**
// FUNCTION :: ANTENNA_SteerableInitFromConfigFile
// LAYER :: PHYSICAL
// PURPOSE :: Init the steerable antennas from
//            configuration file.
// PARAMETERS ::
// + node : Node* : Node pointer that the antenna is being
//                  instantiated in
// + phyIndex : int : interface for which physical layer is
//                    to be initialized
// + nodeInput : const NodeInput* : Pointer to NodeInput structure
// RETURN :: void : NULL
// **/

void ANTENNA_SteerableInitFromConfigFile(
     Node* node,
     int phyIndex,
     const NodeInput* nodeInput);

// /**
// FUNCTION :: AntennaSteerableOmnidirectionalPattern
// LAYER :: PHYSICAL
// PURPOSE :: Is using omnidirectional pattern.
// PARAMETERS ::
// + phyData : PhyData* : Pointer to PhyData structure
// RETURN :: BOOL  : Return TRUE if patternIndex is
//                   ANTENNA_OMNIDIRECTIONAL_PATTERN
// **/


BOOL AntennaSteerableOmnidirectionalPattern(PhyData* phyData);


// /**
// FUNCTION :: AntennaSteerableReturnPatternIndex:
// LAYER :: PHYSICAL
// PURPOSE :: Return current pattern index.
// PARAMETERS ::
// + node : Node* : Node pointer that the antenna is being
//                  instantiated in
// + phyIndex : int : interface for which physical layer is
//                  to be initialized
// RETURN :: int : returns pattern index
// **/

int AntennaSteerableReturnPatternIndex(Node* node, int phyIndex);


// /**
// FUNCTION :: AntennaSteerableGainForThisDirection:
// LAYER :: PHYSICAL
// PURPOSE :: Return gain for this direction.
// PARAMETERS ::
// + node : Node* : Node pointer that the antenna is being
//                 instantiated in
// + phyIndex : int : interface for which physical layer is
//                    to be initialized
// + DOA : Orientation : Direction of arrival
// RETURN :: float : return gain in dBi
// **/

float AntennaSteerableGainForThisDirection(
      Node* node,
      int phyIndex,
      Orientation DOA);


// /**
// FUNCTION :: AntennaSteerableGainForThisDirectionWithPatternIndex
// LAYER :: PHYSICAL
// PURPOSE :: Return gain for current pattern index.
// PARAMETERS ::
// + node : Node* : Node pointer that the antenna is being
//                  instantiated in
// + phyIndex : int : interface for which physical layer is
//                    to be initialized
// + patternIndex : int : Index of a specified antenna pattern
// + DOA : Orientation : Direction of arrival
// RETURN :: float : return gain in dBi
// **/

float AntennaSteerableGainForThisDirectionWithPatternIndex(
      Node* node,
      int phyIndex,
      int patternIndex,
      Orientation DOA);


// /**
// FUNCTION :: AntennaSteerableSetPattern
// LAYER :: PHYSICAL
// PURPOSE :: Set the current pattern.
// PARAMETERS ::
// + node : Node* : Node pointer that the antenna is being
//                  instantiated in
// + phyIndex : int : interface for which physical layer is
//                    to be initialized
// + patternIndex : int : Index of a specified antenna pattern
// RETURN :: void : NULL
// **/

void AntennaSteerableSetPattern(
     Node* node,
     int phyIndex,
     int patternIndex);


// /**
// FUNCTION :: AntennaSteerableGetPattern
// LAYER :: PHYSICAL
// PURPOSE :: Get the current pattern.
// PARAMETERS ::
// + node : Node* : Node pointer that the antenna is being
//                  instantiated in
// + phyIndex : int : interface for which physical layer is
//                    to be initialized
// + patternIndex : int* : Index of a specified antenna pattern
// RETURN :: void : NULL
// **/

void AntennaSteerableGetPattern(
     Node* node,
     int phyIndex,
     int* patternIndex);


// /**
// FUNCTION :: AntennaSteerableSetMaxGainSteeringAngle
// LAYER :: PHYSICAL
// PURPOSE :: Set the steering angle.
// PARAMETERS ::
// + node : Node* : Node pointer that the antenna is being
//                  instantiated in
// + phyIndex : int : interface for which physical layer is
//                    to be initialized
// + DOA : Orientation : Direction of Arrival
// RETURN :: void : NULL
// **/

void AntennaSteerableSetMaxGainSteeringAngle(
    Node* node,
    int phyIndex,
    Orientation DOA);


// /**
// FUNCTION  :: AntennaSteerableGetSteeringAngle
// LAYER :: PHYSICAL
// PURPOSE :: Get the steering angle.
// PARAMETERS ::
// + node : Node* : Node pointer that the antenna is being
//                  instantiated in
// + phyIndex : int : interface for which physical layer is
//                    to be initialized
// + steeringAngle : Orientation* : Steering angle
// RETURN :: void : NULL
// **/

void AntennaSteerableGetSteeringAngle(
     Node* node,
     int phyIndex,
     Orientation* steeringAngle);


// /**
// FUNCTION :: AntennaSteerableMaxGainPatternForThisSignal
// LAYER :: PHYSICAL
// PURPOSE :: Return heighest gain and pattern index
//            for a signal
// PARAMETERS ::
// + node : Node* : Node pointer that the antenna is being
//                  instantiated in
// + phyIndex : int : interface for which physical layer is
//                  to be initialized
// + propRxInfo : PropRxInfo* : Pointer to PropRxInfo structure
// + patternIndex : int* : Index of a specified antenna pattern
// + gain_dBi : float* : Store antenna gain
// RETURN :: void : NULL
// **/

void AntennaSteerableMaxGainPatternForThisSignal(
    Node* node,
    int phyIndex,
    PropRxInfo* propRxInfo,
    int* patternIndex,
    float* gain_dBi);


// /**
// FUNCTION :: AntennaSteerableMaxGainSteeringAngleForThisSignal
// LAYER :: PHYSICAL
// PURPOSE :: Return max gain and steering angle
//            for a signal
// PARAMETERS ::
// + node : Node* : Node pointer that the antenna is being
//                  instantiated in
// + phyIndex : int : interface for which physical layer is
//                  to be initialized
// + DOA : Orientation : Direction of Arrival
// + steeringAngle : Orientation* : Steering angle
// RETURN :: void
// **/

void AntennaSteerableMaxGainSteeringAngleForThisSignal(
    Node* node,
    int phyIndex,
    Orientation DOA,
    Orientation* steeringAngle);

// /**
// FUNCTION :: AntennaSteerableIsLocked
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
BOOL AntennaSteerableIsLocked(
    Node* node,
    int phyIndex)
{
    PhyData* phyData = node->phyData[phyIndex];
    AntennaSteerable* steerable =
        (AntennaSteerable* )phyData->antennaData->antennaVar;

    ERROR_Assert(phyData->antennaData->antennaModelType ==
        ANTENNA_STEERABLE ,
            "antennaModelType not Steerable.\n");
    return (steerable->antennaIsLocked);
}

// /**
// FUNCTION :: AntennaSteerableLockAntennaDirection 
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
void AntennaSteerableLockAntennaDirection(
    Node* node,
    int phyIndex)
{
    PhyData* phyData = node->phyData[phyIndex];
    AntennaSteerable* steerable =
        (AntennaSteerable* )phyData->antennaData->antennaVar;

    ERROR_Assert(phyData->antennaData->antennaModelType ==
        ANTENNA_STEERABLE ,
            "antennaModelType not Steerable.\n");

    steerable->antennaIsLocked = TRUE;
}

// /**
// FUNCTION :: AntennaSteerableUnlockAntennaDirection 
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
void AntennaSteerableUnlockAntennaDirection(
    Node* node,
    int phyIndex)
{
    PhyData* phyData = node->phyData[phyIndex];
    AntennaSteerable* steerable =
        (AntennaSteerable* )phyData->antennaData->antennaVar;

    ERROR_Assert(phyData->antennaData->antennaModelType ==
        ANTENNA_STEERABLE ,
            "antennaModelType not Steerable.\n");

    steerable->antennaIsLocked = FALSE;
}

// /**
// FUNCTION :: AntennaSteerableSetBestConfigurationForAzimuth 
// LAYER :: PHYSICAL
// PURPOSE :: Sets the current antenna-azimuth the the new value that is
//            received as an input parameter
// PARAMETERS ::
// + node : Node* : Node pointer that the antenna is being
//                  instantiated in
// + phyIndex : int : interface for which physical layer is
//                  to be initialized
// + azimuth : double : New value of the antenna's azimuthal Angle 
// RETURN :: void
// **/
static //inline//
void AntennaSteerableSetBestConfigurationForAzimuth(
    Node* node,
    int phyIndex,
    double azimuth)
{
    Orientation anOrientation;

    anOrientation.azimuth = (OrientationType)azimuth;
    anOrientation.elevation = 0;
    AntennaSteerableSetMaxGainSteeringAngle(node, phyIndex, anOrientation);
}

#endif /*ANTENNA_STEERABLE_H*/


