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
#include "antenna_steerable.h"
#include "antenna_global.h"
#include "antenna_switched.h"

#define DEBUG_STEERABLE 0


// /**
// FUNCTION :: AntennaSteerableAlloc
// LAYER :: PHYSICAL
// PURPOSE :: Allocate new Steerable struct.
// PARAMETERS :: NONE
// RETURN :: AntennaSteerable* : pointer to steerable antenna structure
// **/

static
AntennaSteerable* AntennaSteerableAlloc(void)
{
    AntennaSteerable* antennaVars =
        (AntennaSteerable* ) MEM_malloc(sizeof(AntennaSteerable));
    ERROR_Assert(antennaVars != NULL, "Memory allocation error!\n");
    memset(antennaVars, 0, sizeof(AntennaSteerable));

    return antennaVars;
}


// /**
// FUNCTION :: ANTENNA_SteerableInitFromConfigFile
// LAYER :: PHYSICAL
// PURPOSE :: Init the steerable antennas from
//            configuration file.
// PARAMETERS ::
// + node : Node* : Node pointer that the antenna is being
//                  instantiated in
// + phyIndex : int : interface for which physical
//                    to be initialized
// + nodeInput : const NodeInput* : Pointer to NodeInput structure
// RETURN :: void : NULL
// **/

void ANTENNA_SteerableInitFromConfigFile(
     Node* node,
     int phyIndex,
     const NodeInput* nodeInput)
{

    PhyData* phyData = node->phyData[phyIndex];
    BOOL wasFound;
    char antennaPatternName[2 * MAX_STRING_LENGTH];
    float antennaGain_dB;
    float antennaHeight;

    AntennaSteerable* steerable = AntennaSteerableAlloc();
    phyData->antennaData = (AntennaModel* )MEM_malloc(sizeof(AntennaModel));
    memset(phyData->antennaData, 0, sizeof(AntennaModel));


    phyData->antennaData->antennaVar = steerable;
    phyData->antennaData->numModels++;
    phyData->antennaData->antennaModelType = ANTENNA_STEERABLE;


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
        steerable->antennaGain_dB = (float) antennaGain_dB;
    }
    else
    {
        steerable->antennaGain_dB = ANTENNA_DEFAULT_GAIN_dBi;
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
        steerable->antennaHeight = (float)antennaHeight;
    }
    else
    {
        steerable->antennaHeight = ANTENNA_DEFAULT_HEIGHT;
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
                steerable->antennaPatterns =
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
            steerable->antennaPatterns =
                ANTENNA_GlobalAntennaPatternGet(node->partitionData,
                antennaPatternName);

            // Create a new global antenna structure
            if (steerable->antennaPatterns == NULL)
            {
                ANTENNA_GlobalAntennaPatternInitFromConfigFile(node,
                    phyIndex, antennaPatternName,FALSE);
                steerable->antennaPatterns =
                ANTENNA_GlobalAntennaPatternGet(node->partitionData,
                    antennaPatternName);
            }
        }


    ERROR_Assert(steerable->antennaPatterns != NULL,
                       "Antenna initialization error!\n");

    steerable->numPatterns =
        steerable->antennaPatterns->numOfPatterns;
    steerable->patternSetRepeatAngle =
        steerable->antennaPatterns->patternSetRepeatAngle;
    steerable->patternIndex = ANTENNA_OMNIDIRECTIONAL_PATTERN;
    steerable->antennaIsLocked = FALSE;
    steerable->steeringAngle.azimuth = 0;
    steerable->steeringAngle.elevation = 0;
    phyData->antennaData->antennaPatternType
        = steerable->antennaPatterns->antennaPatternType;

    return;
}


// /**
// FUNCTION :: ANTENNA_SteerableInit
// LAYER :: PHYSICAL
// PURPOSE :: Init the steerable antennas from
//            antenna-model config file.
// PARAMETERS
// + node : Node* : Node pointer that the antenna is being
//                  instantiated in
// + nodeInput : const NodeInput* : pointer to node input
// + phyIndex : int : interface for which physical
//                  to be initialized
// + antennaModel : const AntennaModelGlobal* : pointer to Global
//                                              antenna model structure
// RETURN :: void : NULL
// **/

void ANTENNA_SteerableInit(
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

    AntennaSteerable* steerable = AntennaSteerableAlloc();

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
        steerable->antennaGain_dB = (float)atof(buf);
    }
    else
    {
        steerable->antennaGain_dB
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
        steerable->antennaHeight = (float)atof(buf);
    }
    else
    {
        steerable->antennaHeight
            = antennaModel->height;
    }
    steerable->numPatterns = antennaModel->antennaPatterns->numOfPatterns;
    steerable->patternSetRepeatAngle =
        antennaModel->antennaPatterns->patternSetRepeatAngle;
    steerable->antennaPatterns = antennaModel->antennaPatterns;
    steerable->patternIndex = ANTENNA_OMNIDIRECTIONAL_PATTERN;
    steerable->antennaIsLocked = FALSE;
    steerable->steeringAngle.azimuth = 0;
    steerable->steeringAngle.elevation = 0;

    if (DEBUG_STEERABLE)
    {
           printf("PARAMETERS STORED IN STEERABLE STRUCTURE\n");
           printf("HEIGHT %f\tGAIN %f\tNUM_PATTERNS %d\t \
           REPEAT_ANGLE %d\n",
           steerable->antennaHeight, steerable->antennaGain_dB,
           steerable->numPatterns, steerable->patternSetRepeatAngle);
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

    phyData->antennaData->antennaVar = steerable;
    phyData->antennaData->antennaModelType =
        antennaModel->antennaModelType;
    phyData->antennaData->antennaPatternType =
        steerable->antennaPatterns->antennaPatternType;
    phyData->antennaData->numModels++;

}


// /**
// FUNCTION :: AntennaSteerableOmnidirectionalPattern
// LAYER :: PHYSICAL
// PURPOSE :: Is using omnidirectional pattern.
// PARAMETERS ::
// + phyData : PhyData* : Pointer to PhyData structure
// RETURN :: BOOL : Return TRUE if patternIndex is
//                  ANTENNA_OMNIDIRECTIONAL_PATTERN
// **/

BOOL AntennaSteerableOmnidirectionalPattern(
    PhyData* phyData)
{
    AntennaSteerable* steerable =
        (AntennaSteerable* )phyData->antennaData->antennaVar;

    ERROR_Assert(steerable->patternIndex != ANTENNA_PATTERN_NOT_SET ,
        "pattern index is ANTENNA_PATTERN_NOT_SET.\n");

    if (steerable->patternIndex == ANTENNA_OMNIDIRECTIONAL_PATTERN)
    {
        return TRUE;
    }
    return FALSE;
}


// /**
// FUNCTION :: AntennaSteerableReturnPatternIndex:
// LAYER :: PHYSICAL
// PURPOSE :: Return current pattern index.
// PARAMETERS ::
// + node : Node* : Node pointer that the antenna is being
//                  instantiated in
// + phyIndex : int : interface for which physical
//                  to be initialized
// RETURN :: int : returns pattern index
// **/

int AntennaSteerableReturnPatternIndex(
    Node* node,
    int phyIndex)
{
    PhyData* phyData = node->phyData[phyIndex];
    AntennaSteerable* steerable =
        (AntennaSteerable* )phyData->antennaData->antennaVar;

    return steerable->patternIndex;
}


// /**
// FUNCTION :: AntennaSteerableGainForThisDirection:
// LAYER :: PHYSICAL
// PURPOSE :: Return gain for this direction.
// PARAMETERS ::
// + node : Node* : Node pointer that the antenna is being
//                 instantiated in
// + phyIndex : int : interface for which physical
//                    to be initialized
// + DOA : Orientation : Direction of arrival
// RETURN :: float : return gain in dBi
// **/

float AntennaSteerableGainForThisDirection(
      Node* node,
      int phyIndex,
      Orientation DOA)
{
    PhyData* phyData = node->phyData[phyIndex];
    AntennaSteerable* steerable =
        (AntennaSteerable* )phyData->antennaData->antennaVar;
    return AntennaSteerableGainForThisDirectionWithPatternIndex(node,
                                                    phyIndex,
                                                    steerable->patternIndex,
                                                    DOA);
}


// /**
// FUNCTION :: AntennaSteerableGainForThisDirectionWithPatternIndex
// LAYER :: PHYSICAL
// PURPOSE :: Return gain for current pattern index.
// PARAMETERS ::
// + node : Node* : Node pointer that the antenna is being
//                  instantiated in
// + phyIndex : int : interface for which physical
//                    to be initialized
// + patternIndex : int : Index of a specified antenna pattern
// + DOA : Orientation : Direction of arrival
// RETURN :: float : return gain in dBi
// **/

float AntennaSteerableGainForThisDirectionWithPatternIndex(
      Node* node,
      int phyIndex,
      int patternIndex,
      Orientation DOA)
{
    PhyData* phyData = node->phyData[phyIndex];
    AntennaSteerable* steerable =
        (AntennaSteerable* )phyData->antennaData->antennaVar;

    Orientation nodeOrientation;
    Orientation orientation;
    int aziAngleIndex;
    int eleAngleIndex;
    float aziAngle;
    float eleAngle;
    float antennaMaxGain_dBi = ANTENNA_LOWEST_GAIN_dBi;

    ERROR_Assert(patternIndex != ANTENNA_PATTERN_NOT_SET ,
        "pattern index is ANTENNA_PATTERN_NOT_SET.\n");

    if (patternIndex == ANTENNA_OMNIDIRECTIONAL_PATTERN)
    {
        return steerable->antennaGain_dB;
    }

    MOBILITY_ReturnOrientation(node, &nodeOrientation);
    orientation.azimuth = nodeOrientation.azimuth
                                + phyData->antennaMountingAngle.azimuth;
    orientation.elevation = nodeOrientation.elevation
                                + phyData->antennaMountingAngle.elevation;

    if (steerable->antennaPatterns->is3DArray)
    {
        aziAngle = (float) (DOA.azimuth - orientation.azimuth
                        - steerable->steeringAngle.azimuth);
        aziAngle = (float) COORD_NormalizeAzimuthAngle ((int)aziAngle);

        eleAngle = (float) (DOA.elevation - orientation.elevation
                - steerable->steeringAngle.elevation);
        eleAngle = (float)  COORD_NormalizeElevationAngle ((int)eleAngle);

        if (fabs(eleAngle) > 90)
        {
            aziAngle += ANGLE_RESOLUTION/2;
            eleAngle = ANGLE_RESOLUTION/2 - eleAngle;
            aziAngle = (float) COORD_NormalizeAzimuthAngle ((int)aziAngle);
            eleAngle = (float) COORD_NormalizeElevationAngle ((int)eleAngle);
        }

        if (!(steerable->antennaPatterns->is3DGeometry))
        {
            if (aziAngle > 90 && aziAngle < 270)
            {
                eleAngle = ANGLE_RESOLUTION/2 - eleAngle;
                eleAngle = (float)
                    COORD_NormalizeElevationAngle ((int)eleAngle);
            }
        }

        aziAngleIndex = (int)(((float)steerable->antennaPatterns->
                            azimuthResolution / 360) * aziAngle);

        eleAngleIndex = (int)(((float) steerable->antennaPatterns->
                            elevationResolution / 180) *
                            (eleAngle + ANGLE_RESOLUTION/4));
        float*** image =
            (float***) steerable->antennaPatterns
                ->antennaPatternElements;

        antennaMaxGain_dBi =
            image[patternIndex][eleAngleIndex][aziAngleIndex];

        return antennaMaxGain_dBi;
    }

    AntennaPatternElement** element = (AntennaPatternElement** )
        steerable->antennaPatterns->antennaPatternElements;

        aziAngle = (float) (DOA.azimuth - orientation.azimuth
                        - steerable->steeringAngle.azimuth);
    aziAngle = (float) COORD_NormalizeAzimuthAngle ((int)aziAngle);

    eleAngle = (float) (DOA.elevation - orientation.elevation
                             - steerable->steeringAngle.elevation);
    eleAngle = (float)  COORD_NormalizeElevationAngle ((int)eleAngle);

    if (fabs(eleAngle) > 90 && element[STEERABLE_ELEVATION_INDEX])
    {
        aziAngle += ANGLE_RESOLUTION/2;
        eleAngle = ANGLE_RESOLUTION/2 - eleAngle;
        aziAngle = (float) COORD_NormalizeAzimuthAngle ((int)aziAngle);
        eleAngle = (float)  COORD_NormalizeElevationAngle ((int)eleAngle);
    }

    aziAngleIndex = (int)(((float)steerable->antennaPatterns->
                        azimuthResolution / 360) * aziAngle);

    antennaMaxGain_dBi =
        element[STEERABLE_AZIMUTH_INDEX][patternIndex].gains
            [aziAngleIndex];


    if (element[STEERABLE_ELEVATION_INDEX] != NULL)
    {
        if ((steerable->antennaPatterns->is3DGeometry))
        {
            eleAngleIndex = (int)(((float) steerable->antennaPatterns->
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
            eleAngleIndex = (int)(((float) steerable->antennaPatterns->
                        elevationResolution / 360) *
                        (eleAngle + ANGLE_RESOLUTION/2));
        }

        antennaMaxGain_dBi +=
            element[STEERABLE_ELEVATION_INDEX][
            patternIndex].gains[eleAngleIndex];
    }

    return antennaMaxGain_dBi;
}


// /**
// FUNCTION :: AntennaSteerableSetPattern
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

void AntennaSteerableSetPattern(
     Node* node,
     int phyIndex,
     int patternIndex)
{
    PhyData* phyData = node->phyData[phyIndex];
    AntennaSteerable* steerable =
        (AntennaSteerable* )phyData->antennaData->antennaVar;

    ERROR_Assert(phyData->antennaData->antennaModelType ==
        ANTENNA_STEERABLE ,
            "antennaModelType not Steerable.\n");
    ERROR_Assert(patternIndex < steerable->numPatterns ,
        "Illegal pattern index.\n");

    steerable->patternIndex = patternIndex;

    return;
}


// /**
// FUNCTION :: AntennaSteerableGetPattern
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

void AntennaSteerableGetPattern(
     Node* node,
     int phyIndex,
     int* patternIndex)
{
    PhyData* phyData = node->phyData[phyIndex];
    AntennaSteerable* steerable =
        (AntennaSteerable* )phyData->antennaData->antennaVar;


    ERROR_Assert(phyData->antennaData->antennaModelType ==
        ANTENNA_STEERABLE ,
            "antennaModelType is not Steerable.\n");

    *patternIndex = steerable->patternIndex;

    return;
}


// /**
// FUNCTION :: AntennaSteerableSetMaxGainSteeringAngle
// LAYER :: PHYSICAL
// PURPOSE :: Set the steering angle.
// PARAMETERS ::
// + node : Node* : Node pointer that the antenna is being
//                  instantiated in
// + phyIndex : int : interface for which physical
//                    to be initialized
// + DOA : Orientation : Direction of arrival
// RETURN :: void : NULL
// **/

void AntennaSteerableSetMaxGainSteeringAngle(
     Node* node,
     int phyIndex,
     Orientation DOA)
{
    PhyData* phyData = node->phyData[phyIndex];
    AntennaSteerable* steerable =
        (AntennaSteerable* )phyData->antennaData->antennaVar;
    Orientation steeringAngle;
    int patternIndex = 1;

    ERROR_Assert(phyData->antennaData->antennaModelType ==
        ANTENNA_STEERABLE ,
            "antennaModelType is not Steerable.\n");

    if (steerable->numPatterns == 1)
    {
        AntennaSteerableSetPattern(node, phyIndex, 0);
    } else
    {
        const int patternSectorAngle =
           steerable->patternSetRepeatAngle / steerable->numPatterns;
        patternIndex =
           ((DOA.azimuth + (patternSectorAngle/2)) %
             steerable->patternSetRepeatAngle) / patternSectorAngle;

        AntennaSteerableSetPattern(node, phyIndex, patternIndex);
    }//if//

    AntennaSteerableMaxGainSteeringAngleForThisSignal(node,
                                                      phyIndex,
                                                      DOA,
                                                      &steeringAngle);

    steerable->steeringAngle = steeringAngle;

    return;
}


// /**
// FUNCTION :: AntennaSteerableGetSteeringAngle
// LAYER :: PHYSICAL
// PURPOSE :: Get the steering angle.
// PARAMETERS ::
// + node : Node* : Node pointer that the antenna is being
//                  instantiated in
// + phyIndex : int : interface for which physical
//                    to be initialized
// + steeringAngle : Orientation* : Steering angle
// RETURN :: void : NULL
// **/

void AntennaSteerableGetSteeringAngle(
     Node *node,
     int phyIndex,
     Orientation* steeringAngle)
{
    PhyData* phyData = node->phyData[phyIndex];
    AntennaSteerable* steerable =
        (AntennaSteerable* )phyData->antennaData->antennaVar;

    ERROR_Assert(phyData->antennaData->antennaModelType ==
        ANTENNA_STEERABLE ,
            "antennaModelType not Steerable.\n");

    *steeringAngle = steerable->steeringAngle;

    return;
}


// /**
// FUNCTION :: AntennaSteerableMaxGainPatternForThisSignal
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

void AntennaSteerableMaxGainPatternForThisSignal(
     Node* node,
     int phyIndex,
     PropRxInfo* propRxInfo,
     int* patternIndex,
     float* gain_dBi)
{
    PhyData* phyData = node->phyData[phyIndex];
    AntennaSteerable* steerable =
        (AntennaSteerable* )phyData->antennaData->antennaVar;
    int i;
    int highestGainPattern = ANTENNA_PATTERN_NOT_SET;
    float highestGain_dBi = ANTENNA_LOWEST_GAIN_dBi;

    ERROR_Assert(phyData->antennaData->antennaModelType ==
        ANTENNA_STEERABLE ,
            "antennaModelType not Steerable.\n");

    for (i = 0; i < steerable->numPatterns; i++)
    {
        if (steerable->antennaPatterns->boreSightGain_dBi[i]
            > highestGain_dBi)
        {
            highestGainPattern = i;
            highestGain_dBi =
                steerable->antennaPatterns->boreSightGain_dBi[i];
        }
    }

    if (highestGain_dBi < steerable->antennaGain_dB)
    {
        *patternIndex = ANTENNA_OMNIDIRECTIONAL_PATTERN;
        *gain_dBi = steerable->antennaGain_dB;
    }
    else
    {
        *patternIndex = highestGainPattern;
        *gain_dBi = highestGain_dBi;
    }

    return;
}


// /**
// FUNCTION :: AntennaSteerableMaxGainSteeringAngleForThisSignal
// LAYER :: PHYSICAL
// PURPOSE :: Return max gain and steering angle
//            for a signal
// PARAMETERS
// + node : Node* : Node pointer that the antenna is being
//                  instantiated in
// + phyIndex : int : interface for which physical
//                    to be initialized
// + DOA : Orientation : Direction of Arrival
// + steeringAngle : Orientation* : Steering angle
// RETURN :: void : NULL
// **/

void AntennaSteerableMaxGainSteeringAngleForThisSignal(
     Node* node,
     int phyIndex,
     Orientation DOA,
     Orientation* steeringAngle)
{
    PhyData* phyData = node->phyData[phyIndex];
    AntennaSteerable* steerable =
        (AntennaSteerable* )phyData->antennaData->antennaVar;
    Orientation orientation;
    Orientation nodeOrientation;
    Orientation boreSightAngle = steerable->antennaPatterns->
                        boreSightAngle[steerable->patternIndex];
    ERROR_Assert(phyData->antennaData->antennaModelType ==
        ANTENNA_STEERABLE ,"antennaModelType is not steerable.\n");

    if (!(steerable->antennaPatterns->is3DGeometry)&&
            (boreSightAngle.elevation > 90 && boreSightAngle.azimuth > 90
            && boreSightAngle.azimuth < 270))
    {
        boreSightAngle.elevation = ANGLE_RESOLUTION/2 -
                                    boreSightAngle.elevation;
        boreSightAngle.elevation = (OrientationType)
            COORD_NormalizeElevationAngle (boreSightAngle.elevation);
    }


    MOBILITY_ReturnOrientation(node, &nodeOrientation);

    orientation.azimuth = nodeOrientation.azimuth
                                + phyData->antennaMountingAngle.azimuth;
    orientation.elevation = nodeOrientation.elevation
                                + phyData->antennaMountingAngle.elevation;

    steeringAngle->azimuth = (short)
        COORD_NormalizeAzimuthAngle(
            DOA.azimuth -
            orientation.azimuth -
        boreSightAngle.azimuth);

    steeringAngle->elevation = (short)
        COORD_NormalizeElevationAngle(
            DOA.elevation -
            orientation.elevation -
        boreSightAngle.elevation);

        if (abs(steeringAngle->elevation) > 90)
        {
            steeringAngle->azimuth += ANGLE_RESOLUTION/2;
            steeringAngle->elevation =
                ANGLE_RESOLUTION/2 - steeringAngle->elevation;
            steeringAngle->azimuth = (OrientationType)
                COORD_NormalizeAzimuthAngle (steeringAngle->azimuth);
            steeringAngle->elevation = (OrientationType)
                COORD_NormalizeElevationAngle (steeringAngle->elevation);
        }
    return;
}

