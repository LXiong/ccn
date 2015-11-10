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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "partition.h"
#include "antenna.h"
#include "antenna_switched.h"
#include "antenna_global.h"

#define DEBUG_SWITCHED_BEAM 0


// /**
// FUNCTION :: AntennaSwitchedBeamAlloc
// LAYER :: PHYSICAL
// PURPOSE :: Allocate new SwitchedBeam struct.
// PARAMETERS :: NONE
// RETURN :: AntennaSwitchedBeam* : pointer to switchedbeam antenna
// **/

static
AntennaSwitchedBeam* AntennaSwitchedBeamAlloc(void)
{
    AntennaSwitchedBeam* antennaVars =
        (AntennaSwitchedBeam* ) MEM_malloc(sizeof(AntennaSwitchedBeam));
    ERROR_Assert(antennaVars != NULL, "Memory allocation error!\n");
    memset(antennaVars, 0, sizeof(AntennaSwitchedBeam));

    return antennaVars;
}


// /**
// FUNCTION :: ANTENNA_SwitchedBeamInitFromConfigFile
// LAYER :: PHYSICAL
// PURPOSE :: Initialize data structure.
// PARAMETERS ::
// + node : Node* : Node pointer that the antenna is being
//                  instantiated in
// + phyIndex : int : interface for which physical
//                    to be initialized
// + nodeInput : const NodeInput* : Pointer to NodeInput structure
// RETURN :: void
// **/

void ANTENNA_SwitchedBeamInitFromConfigFile(
    Node* node,
    int phyIndex,
    const NodeInput* nodeInput)
{
    PhyData* phyData = node->phyData[phyIndex];
    BOOL wasFound;
    char antennaPatternName[2 * MAX_STRING_LENGTH];
    float antennaGain_dB;
    float antennaHeight;

    AntennaSwitchedBeam* switched = AntennaSwitchedBeamAlloc();
    phyData->antennaData =
        (AntennaModel* )MEM_malloc(sizeof(AntennaModel));
    memset(phyData->antennaData, 0, sizeof(AntennaModel));


    phyData->antennaData->antennaVar = switched;
    phyData->antennaData->numModels++;
    phyData->antennaData->antennaModelType = ANTENNA_SWITCHED_BEAM;

    // Set ANTENNA-GAIN

    IO_ReadFloat(
        node,
        node->nodeId,
        phyData->macInterfaceIndex,
        nodeInput,
        "ANTENNA-GAIN",
        &wasFound,
        &antennaGain_dB);

    if (wasFound)
    {
        switched->antennaGain_dB = (float) antennaGain_dB;
    }
    else
    {
        switched->antennaGain_dB = ANTENNA_DEFAULT_GAIN_dBi;
    }

    IO_ReadFloat(
        node,
        node->nodeId,
        phyData->macInterfaceIndex,
        nodeInput,
        "ANTENNA-HEIGHT",
        &wasFound,
        &antennaHeight);

    if (wasFound)
    {
        ERROR_Assert(antennaHeight >= 0,
            "Antenna Height can not be negative.\n");
        switched->antennaHeight = antennaHeight;
    }
    else
    {
        switched->antennaHeight = ANTENNA_DEFAULT_HEIGHT;
    }

        char    buf[MAX_STRING_LENGTH];

        IO_ReadString(
            node,
            node->nodeId,
            phyData->macInterfaceIndex,
            nodeInput,
            "ANTENNA-PATTERN-TYPE",
            &wasFound,
            buf);

        if (wasFound)
        {
            switched->antennaPatterns =
            ANTENNA_GlobalModelAssignPattern(node,
            phyIndex,
            nodeInput);

        }
        else
        {
            phyData->antennaData->antennaPatternType =
                                ANTENNA_PATTERN_TRADITIONAL;
            const char* buf = "TRADITIONAL";
            ANTENNA_GeneratePatterName(node,
                                        phyIndex,
                                        nodeInput,
                                        buf,
                                        antennaPatternName);

            // find the global antenna structure for this type
            switched->antennaPatterns =
                ANTENNA_GlobalAntennaPatternGet(node->partitionData,
                antennaPatternName);

            // Create a new global antenna structure
            if (switched->antennaPatterns == NULL)
            {
                ANTENNA_GlobalAntennaPatternInitFromConfigFile(node,
                phyIndex, antennaPatternName,FALSE);
                switched->antennaPatterns =
                ANTENNA_GlobalAntennaPatternGet(node->partitionData,
                antennaPatternName);
            }
        }


    ERROR_Assert(switched->antennaPatterns != NULL,
                       "Antenna initialization error!\n");

    switched->numPatterns =
        switched->antennaPatterns->numOfPatterns;
    switched->lastPatternIndex = ANTENNA_OMNIDIRECTIONAL_PATTERN;
    switched->patternIndex = ANTENNA_OMNIDIRECTIONAL_PATTERN;
    switched->antennaIsLocked = FALSE;
    phyData->antennaData->antennaPatternType
        = switched->antennaPatterns->antennaPatternType;

    return;
}


// /**
// FUNCTION :: ANTENNA_SwitchedBeamInit
// LAYER :: PHYSICAL
// PURPOSE :: Init the switchedbeam antennas from
//            antenna-model config file.
// PARAMETERS::
// + node : Node* : Node pointer that the antenna is being
//                  instantiated in
// + nodeInput : const NodeInput* : pointer to node input
// + phyIndex : int : interface for which physical
//                    to be initialized
// + antennaModel : const AntennaModelGlobal* : pointer to Global
//                                              antenna model structure
// RETURN :: void : NULL
// **/

void ANTENNA_SwitchedBeamInit(
    Node* node,
    const NodeInput* nodeInput,
    int phyIndex,
    const  AntennaModelGlobal* antennaModel)
{

    PhyData* phyData = node->phyData[phyIndex];
    char buf[MAX_STRING_LENGTH];
    BOOL wasFound;
    int antennaModelMatchType;
    int matchType;
    double systemLoss_dB = 0.0;
    NodeAddress ipv4address;
    Address ipv6Address;

    AntennaSwitchedBeam* switched = AntennaSwitchedBeamAlloc();

    ipv4address = MAPPING_GetInterfaceAddressForInterface(
                    node, node->nodeId, phyData->macInterfaceIndex);

    ipv6Address = MAPPING_GetInterfaceAddressForInterface(
                     node, node->nodeId, NETWORK_IPV6, phyData->macInterfaceIndex);

    // init structure
    IO_ReadString(
        node->nodeId,
        phyData->macInterfaceIndex,
        ipv4address,
        &ipv6Address.interfaceAddr.ipv6,
        nodeInput,
        "ANTENNA-MODEL",
        buf,
        wasFound,
        antennaModelMatchType);

    IO_ReadString(
        node->nodeId,
        phyData->macInterfaceIndex,
        ipv4address,
        &ipv6Address.interfaceAddr.ipv6,
        nodeInput,
        "ANTENNA-GAIN",
        buf,
        wasFound,
        matchType);

    if (wasFound && matchType >= antennaModelMatchType)
    {
        switched->antennaGain_dB = (float)atof(buf);
    }
    else
    {
        switched->antennaGain_dB
            = antennaModel->antennaGain_dB;
    }

    IO_ReadString(
        node->nodeId,
        phyData->macInterfaceIndex,
        ipv4address,
        &ipv6Address.interfaceAddr.ipv6,
        nodeInput,
        "ANTENNA-HEIGHT",
        buf,
        wasFound,
        matchType);

    if (wasFound && matchType >= antennaModelMatchType)
    {
        switched->antennaHeight = (float)atof(buf);
    }
    else
    {
        switched->antennaHeight
            = antennaModel->height;
    }

    switched->numPatterns = antennaModel->antennaPatterns->numOfPatterns;
    switched->antennaPatterns = antennaModel->antennaPatterns;


    switched->patternIndex = ANTENNA_OMNIDIRECTIONAL_PATTERN;
    switched->lastPatternIndex = ANTENNA_OMNIDIRECTIONAL_PATTERN;
    switched->antennaIsLocked = FALSE;

    if (DEBUG_SWITCHED_BEAM)
    {
       printf("PARAMETERS STORED IN SWITCHED-BEAM STRUCTURE\n");
       printf("HEIGHT %f\tGAIN %f\tNUM_PATTERNS %d\n",
       switched->antennaHeight, switched->antennaGain_dB,
       switched->numPatterns);
    }

    phyData->antennaData =
        (AntennaModel* )MEM_malloc(sizeof(AntennaModel));
    memset(phyData->antennaData, 0, sizeof(AntennaModel));

    IO_ReadString(
        node->nodeId,
        phyData->macInterfaceIndex,
        ipv4address,
        &ipv6Address.interfaceAddr.ipv6,
        nodeInput,
        "ANTENNA-EFFICIENCY",
        buf,
        wasFound,
        matchType);

    if (wasFound && matchType >= antennaModelMatchType)
    {
        systemLoss_dB += -IN_DB(atof(buf));
    }
    else
    {
        systemLoss_dB += antennaModel->antennaEfficiency_dB;
    }

    // Set ANTENNA-MISMATCH-LOSS

    IO_ReadString(
        node->nodeId,
        phyData->macInterfaceIndex,
        ipv4address,
        &ipv6Address.interfaceAddr.ipv6,
        nodeInput,
        "ANTENNA-MISMATCH-LOSS",
        buf,
        wasFound,
        matchType);

    if (wasFound && matchType >= antennaModelMatchType)
    {
        systemLoss_dB += (atof(buf));
    }
    else
    {
        systemLoss_dB += antennaModel->antennaMismatchLoss_dB;
    }
    // Set ANTENNA-CABLE-LOSS

    IO_ReadString(
        node->nodeId,
        phyData->macInterfaceIndex,
        ipv4address,
        &ipv6Address.interfaceAddr.ipv6,
        nodeInput,
        "ANTENNA-CABLE-LOSS",
        buf,
        wasFound,
        matchType);

    if (wasFound && matchType >= antennaModelMatchType)
    {
        systemLoss_dB += (atof(buf));
    }
    else
    {
        systemLoss_dB += antennaModel->antennaCableLoss_dB;
    }

    // Set ANTENNA-CONNECTION-LOSS

    IO_ReadString(
        node->nodeId,
        phyData->macInterfaceIndex,
        ipv4address,
        &ipv6Address.interfaceAddr.ipv6,
        nodeInput,
        "ANTENNA-CONNECTION-LOSS",
        buf,
        wasFound,
        matchType);

    if (wasFound && matchType >= antennaModelMatchType)
    {
        systemLoss_dB += (atof(buf));
    }
    else
    {
        systemLoss_dB += antennaModel->antennaConnectionLoss_dB;
    }

    phyData->systemLoss_dB = systemLoss_dB;
    phyData->antennaData->antennaVar = switched;
    phyData->antennaData->antennaModelType =
        antennaModel->antennaModelType;
    phyData->antennaData->antennaPatternType =
        switched->antennaPatterns->antennaPatternType;
    phyData->antennaData->numModels++;

}


// /**
// FUNCTION :: AntennaSwitchedBeamOmnidirectionalPattern
// LAYER :: PHYSICAL
// PURPOSE :: Is using omnidirectional pattern.
// PARAMETERS::
// + phyData : PhyData* : Pointer to PhyData structure
// RETURN :: BOOL : Return TRUE if patternIndex is
//                  ANTENNA_OMNIDIRECTIONAL_PATTERN
// **/

BOOL AntennaSwitchedBeamOmnidirectionalPattern(
    PhyData* phyData)
{
    AntennaSwitchedBeam* switched =
        (AntennaSwitchedBeam* )phyData->antennaData->antennaVar;

    ERROR_Assert(switched->patternIndex != ANTENNA_PATTERN_NOT_SET ,
        "pattern index is ANTENNA_PATTERN_NOT_SET.\n");

    if (switched->patternIndex == ANTENNA_OMNIDIRECTIONAL_PATTERN)
    {
        return TRUE;
    }
    return FALSE;
}


// /**
// FUNCTION :: AntennaSwitchedBeamReturnPatternIndex
// LAYER :: PHYSICAL
// PURPOSE  :: Return current pattern index.
// PARAMETERS ::
// + node : Node* : Node pointer that the antenna is being
//                  instantiated in
// + phyIndex : int : interface for which physical
//                  to be initialized
// RETURN :: int : Return the patternIndex
// **/

int AntennaSwitchedBeamReturnPatternIndex(
    Node* node,
    int phyIndex)
{
    PhyData* phyData = node->phyData[phyIndex];
    AntennaSwitchedBeam* switched =
        (AntennaSwitchedBeam* )phyData->antennaData->antennaVar;

    return switched->patternIndex;
}


// /**
// FUNCTION       :: AntennaSwitchedBeamGainForThisDirection
// LAYER     :: PHYSICAL
// PURPOSE   :: Return gain for this direction.
// PARAMETERS::
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
    Orientation DOA)
{
    PhyData* phyData = node->phyData[phyIndex];
    AntennaSwitchedBeam* switched =
        (AntennaSwitchedBeam* )phyData->antennaData->antennaVar;
    return AntennaSwitchedBeamGainForThisDirectionWithPatternIndex(node,
                                                    phyIndex,
                                                    switched->patternIndex,
                                                    DOA);
    }

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
// RETURN :: float : Return antennaGain
// **/

float AntennaSwitchedBeamGainForThisDirectionWithPatternIndex(
    Node* node,
    int phyIndex,
    int patternIndex,
    Orientation DOA)
{
    PhyData* phyData = node->phyData[phyIndex];
    AntennaSwitchedBeam* switched =
        (AntennaSwitchedBeam* )phyData->antennaData->antennaVar;
    Orientation nodeOrientation;
    Orientation orientation;

    float aziAngle;
    float eleAngle;
    int aziAngleIndex;
    int eleAngleIndex;
    float antennaMaxGain_dBi = ANTENNA_LOWEST_GAIN_dBi;

    ERROR_Assert(patternIndex != ANTENNA_PATTERN_NOT_SET ,
        "pattern index is ANTENNA_PATTERN_NOT_SET.\n");

    if (patternIndex == ANTENNA_OMNIDIRECTIONAL_PATTERN)
    {
        return switched->antennaGain_dB;
    }

    MOBILITY_ReturnOrientation(node, &nodeOrientation);

    orientation.azimuth = nodeOrientation.azimuth
                                + phyData->antennaMountingAngle.azimuth;
    orientation.elevation = nodeOrientation.elevation
                                + phyData->antennaMountingAngle.elevation;

    if (switched->antennaPatterns->is3DArray)
        {
            aziAngle = (float) (DOA.azimuth - orientation.azimuth);
        aziAngle = (float) COORD_NormalizeAzimuthAngle ((int)aziAngle);

        eleAngle = (float) (DOA.elevation - orientation.elevation);
        eleAngle = (float)  COORD_NormalizeElevationAngle ((int)eleAngle);

        if (fabs(eleAngle) > 90)
        {
            aziAngle += ANGLE_RESOLUTION/2;
            eleAngle = ANGLE_RESOLUTION/2 - eleAngle;
            aziAngle = (float) COORD_NormalizeAzimuthAngle ((int)aziAngle);
            eleAngle = (float) COORD_NormalizeElevationAngle ((int)eleAngle);
        }

        if (!(switched->antennaPatterns->is3DGeometry))
        {
            if (aziAngle > 90 && aziAngle < 270)
            {
                eleAngle = ANGLE_RESOLUTION/2 - eleAngle;
                eleAngle = (float)
                    COORD_NormalizeElevationAngle ((int)eleAngle);
            }
        }

        aziAngleIndex = (int)(((float)switched->antennaPatterns->
                            azimuthResolution / 360) * aziAngle);

            eleAngleIndex = (int)(((float) switched->antennaPatterns->
                            elevationResolution / 180) *
                            (eleAngle + ANGLE_RESOLUTION/4));
            float*** image =
                (float***) switched->antennaPatterns
                    ->antennaPatternElements;

            antennaMaxGain_dBi =
                image[patternIndex][eleAngleIndex][aziAngleIndex];

            return antennaMaxGain_dBi;
    }

    AntennaPatternElement** element = (AntennaPatternElement** )
        switched->antennaPatterns->antennaPatternElements;

    aziAngle = (float) (DOA.azimuth - orientation.azimuth);
    aziAngle = (float) COORD_NormalizeAzimuthAngle ((int)aziAngle);

    eleAngle = (float) (DOA.elevation - orientation.elevation);
    eleAngle = (float)  COORD_NormalizeElevationAngle ((int)eleAngle);

    if (fabs(eleAngle) > 90 && element[SWITCHED_ELEVATION_INDEX])
    {
        aziAngle += ANGLE_RESOLUTION/2;
        eleAngle = ANGLE_RESOLUTION/2 - eleAngle;
        aziAngle = (float) COORD_NormalizeAzimuthAngle ((int)aziAngle);
        eleAngle = (float) COORD_NormalizeElevationAngle ((int)eleAngle);
    }

    aziAngleIndex = (int)(((float)switched->antennaPatterns->
                        azimuthResolution / 360) * aziAngle);

    antennaMaxGain_dBi =
        element[SWITCHED_AZIMUTH_INDEX][patternIndex].gains
            [aziAngleIndex];

    if (element[SWITCHED_ELEVATION_INDEX] != NULL)
    {
        if ((switched->antennaPatterns->is3DGeometry))
        {
            eleAngleIndex = (int)(((float) switched->antennaPatterns->
                                elevationResolution / 180) *
                                (eleAngle + ANGLE_RESOLUTION/4));
        }
        else
        {
            if (aziAngle > 90 && aziAngle < 270)
            {
                eleAngle = ANGLE_RESOLUTION/2 - eleAngle;
                eleAngle = (float)
                    COORD_NormalizeElevationAngle ((int)eleAngle);
            }
            eleAngleIndex = (int)(((float) switched->antennaPatterns->
                        elevationResolution / 360) *
                        (eleAngle + ANGLE_RESOLUTION/2));
        }
        antennaMaxGain_dBi +=
            element[SWITCHED_ELEVATION_INDEX][
            patternIndex].gains[eleAngleIndex];
    }

    return antennaMaxGain_dBi;
}


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
    int patternIndex)
{
    PhyData* phyData = node->phyData[phyIndex];
    AntennaSwitchedBeam* switched =
        (AntennaSwitchedBeam* )phyData->antennaData->antennaVar;

    ERROR_Assert(phyData->antennaData->antennaModelType ==
        ANTENNA_SWITCHED_BEAM ,
            "antennaModelType is not Switched beam.\n");
    ERROR_Assert(patternIndex < switched->numPatterns ,
        "Illegal pattern index.\n");

    ERROR_Assert(switched->numPatterns != 0 ,
        "number of patterns is Zero.\n");
    switched->lastPatternIndex = switched->patternIndex;
    switched->patternIndex = patternIndex;

    return;
}


// /**
// FUNCTION :: AntennaSwitchedBeamGetPattern
// LAYER :: PHYSICAL
// PURPOSE :: Get the current pattern.
// PARAMETERS::
// + node : Node* : Node pointer that the antenna is being
//                  instantiated in
// + phyIndex : int : interface for which physical
//                    to be initialized
// + patternIndex : int : Index of a specified antenna pattern
// RETURN :: void : NULL
// **/

void AntennaSwitchedBeamGetPattern(
    Node* node,
    int phyIndex,
    int* patternIndex)
{
    PhyData* phyData = node->phyData[phyIndex];
    AntennaSwitchedBeam* switched =
        (AntennaSwitchedBeam* )phyData->antennaData->antennaVar;


    ERROR_Assert(phyData->antennaData->antennaModelType ==
        ANTENNA_SWITCHED_BEAM ,
            "antennaModelType is switched beam.\n");

    *patternIndex = switched->patternIndex;

    return;
}


// /**
// FUNCTION :: AntennaSwitchedBeamMaxGainPatternForThisSignal
// LAYER :: PHYSICAL
// PURPOSE :: Return heighest gain and pattern index
//            for a signal
// PARAMETERS::
// + node : Node* : Node pointer that the antenna is being
//                  instantiated in
// + phyIndex : int : interface for which physical
//                    to be initialized
// + patternIndex : int* : Index of a specified antenna pattern
// + propRxInfo : PropRxInfo* : Pointer to PropRxInfo structure
// + gain_dBi : float* : Store antenna gain
// RETURN :: void : NULL
// **/

void AntennaSwitchedBeamMaxGainPatternForThisSignal(
    Node* node,
    int phyIndex,
    PropRxInfo* propRxInfo,
    int* patternIndex,
    float* gain_dBi)
{
    PhyData* phyData = node->phyData[phyIndex];
    AntennaSwitchedBeam* switched =
        (AntennaSwitchedBeam* )phyData->antennaData->antennaVar;

    int i;
    int maxGainPattern = ANTENNA_PATTERN_NOT_SET;
    float maxGain_dBi = ANTENNA_LOWEST_GAIN_dBi;
    const int originalPatternIndex = switched->patternIndex;
    const int numPatterns = switched->numPatterns;

    ERROR_Assert(phyData->antennaData->antennaModelType ==
        ANTENNA_SWITCHED_BEAM ,
            "antennaModelType is switched beam.\n");

    for (i = 0; i < numPatterns; i++)
    {
        float gain_dBi;

        AntennaSwitchedBeamSetPattern(node, phyIndex, i);
        gain_dBi = ANTENNA_GainForThisSignal(node,
            phyIndex, propRxInfo);

        if (gain_dBi > maxGain_dBi)
        {
            maxGainPattern = i;
            maxGain_dBi = gain_dBi;
        }
     }

    if (maxGainPattern == ANTENNA_PATTERN_NOT_SET ||
        maxGain_dBi < switched->antennaGain_dB)
    {
        *patternIndex = ANTENNA_OMNIDIRECTIONAL_PATTERN;
        *gain_dBi = switched->antennaGain_dB;
    }
    else
    {
        *patternIndex = maxGainPattern;
        *gain_dBi = maxGain_dBi;
    }
    switched->patternIndex = originalPatternIndex;

    return;
}

