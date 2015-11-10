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

//
//
// This file defines a common set of functions for all the antenna models
// as well as a set of functions specific to the omni-directional antenna
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "partition.h"
#include "phy.h"
#include "fileio.h"
#include "antenna.h"
#include "antenna_global.h"
#include "antenna_switched.h"
#include "antenna_steerable.h"
#include "antenna_patterned.h"

#define DEBUG_NSMA 0
#define DEBUG_TRADITIONAL 0
#define DEBUG_OMNI 0
#define DEBUG_ASCII3D 0
#define DEBUG_ASCII2D 0

#ifdef AGI_INTERFACE
#include "agi_interface_util.h"
#endif

// /**
// FUNCTION :: AntennaOmnidirectionalAlloc
// LAYER :: Physical Layer.
// PURPOSE :: Allocate new Omnidirectional struct.
// PARAMETERS :: NONE.
// RETURN :: AntennaOmnidirectional* : pointer to
//                                     AntennaOmnidirectional structure
// **/

static
AntennaOmnidirectional* AntennaOmnidirectionalAlloc(void)
{
    AntennaOmnidirectional *antennaVars
        = (AntennaOmnidirectional *) MEM_malloc(
            sizeof(AntennaOmnidirectional));
    ERROR_Assert(antennaVars,"Unable to alloc omnidirectional antenna.\n");
    memset(antennaVars, 0, sizeof(AntennaOmnidirectional));

    return antennaVars;
}


static
void FillArrayValues(
    float *pattern_dB,
    int start,
    int end,
    float value)
{
    int i;

    ERROR_Assert(start >= 0 , "Illegal start value.\n");
    ERROR_Assert(end < ANGLE_RESOLUTION , "Illegal end value.\n");

    for (i = start; i <= end; i++)
    {
        pattern_dB[i] = value;
    }

    return;
}


// /**
// FUNCTION :: ANTENNA_FillGainValues
// LAYER :: Physical Layer.
// PURPOSE :: Fill the discontinuity between angles in radiation
//            pattern file with linear interpolation.
// PARAMETERS ::
// + pattern_dB : float* : array for storing antenna gains.
// + startAngle : int : starting angle of discontinuity.
// + endAngle : int : end angle of discontinuity.
// + preGainValue : float : previously available gain value.
// + nextGainValue : float : next available gain value.
// RETURN :: void : NULL
// **/

static
void ANTENNA_FillGainValues(
    float* pattern_dB,
    int startAngle,
    int endAngle,
    float preGainValue,
    float nextGainValue)
{
    int i;

    float delta;

    ERROR_Assert(startAngle >= 0 , "Illegal start value.\n");
    ERROR_Assert(endAngle <= ANGLE_RESOLUTION , "Illegal end value.\n");

    delta = (nextGainValue - preGainValue)
        / ((endAngle + 1) - (startAngle - 1));

    for (i = startAngle; i <= endAngle; i++)
    {
        preGainValue = preGainValue + delta;
        pattern_dB[i] = preGainValue;
    }
}


// /**
// FUNCTION :: ANTENNA_OmniDirectionalInit
// LAYER :: Physical Layer.
// PURPOSE :: Initialize omnidirectional antenna
//            from the antenna model file.
// PARAMETERS ::
// + node : Node* : node being initialized.
// + phyIndex : int : interface for which physical to be
//                    initialized.
// + antennaModel : const  AntennaModelGlobal* : pointer to AntennaModelGlobal
//                                               structure.
// RETURN :: void : NULL
// **/

void ANTENNA_OmniDirectionalInit(
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
    double systemLoss_dB =0.0;
    NodeAddress ipv4address;
    Address ipv6Address;

    AntennaOmnidirectional* antennaOmnidirectional
        = AntennaOmnidirectionalAlloc();

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
        antennaOmnidirectional->antennaGain_dB = (float)atof(buf);
    }
    else
    {
    antennaOmnidirectional->antennaGain_dB
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
        antennaOmnidirectional->antennaHeight = (float)atof(buf);
    }
    else
    {
    antennaOmnidirectional->antennaHeight
        = antennaModel->height;
    }


    if (DEBUG_OMNI)
    {
        printf("PARAMETERS STORED IN OMNIDIRECTIONAL STRUCTURE\n");
        printf("HEIGHT %f\tGAIN %f\n",
            antennaOmnidirectional->antennaHeight,
            antennaOmnidirectional->antennaGain_dB);
    }

    // Assign antenna model based on Node's model type

    phyData->antennaData
        = (AntennaModel*)MEM_malloc(sizeof(AntennaModel));

    memset(phyData->antennaData, 0, sizeof(AntennaModel));

    phyData->antennaData->antennaVar = antennaOmnidirectional;
    phyData->antennaData->antennaModelType
        = antennaModel->antennaModelType;
    phyData->antennaData->numModels++;

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
}

// /**
// FUNCTION :: ANTENNA_OmniDirectionalInitFromConfigFile
// LAYER :: Physical Layer.
// PURPOSE : Initialize omnidirectional antenna
//           from the default.config file.
// PARAMETERS ::
// + node : Node* : node being initialized.
// + phyIndex : int : interface for which physical to be
//                    initialized.
// + nodeInput : const NodeInput* : structure containing contents of input
//                                  file.
// RETURN :: void : NULL
// **/

void ANTENNA_OmniDirectionalInitFromConfigFile(
    Node* node,
    int phyIndex,
    const NodeInput* nodeInput)
{

    PhyData* phyData = node->phyData[phyIndex];
    BOOL wasFound;
    float antennaGain_dB;
    float antennaHeight;

    phyData->antennaData =
        (AntennaModel*) MEM_malloc(sizeof(AntennaModel));
    memset(phyData->antennaData, 0, sizeof(AntennaModel));
    AntennaOmnidirectional* antennaOmnidirectional
        = AntennaOmnidirectionalAlloc();
    //
    // Set ANTENNA-GAIN
    //
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
        antennaOmnidirectional->antennaGain_dB
            = (float)antennaGain_dB;
    }
    else
    {
        antennaOmnidirectional->antennaGain_dB
            = ANTENNA_DEFAULT_GAIN_dBi;
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
        ERROR_Assert(antennaHeight >= 0 ,
            "antenna height given negative.\n");
        antennaOmnidirectional->antennaHeight
            = (float)antennaHeight;
    }
    else
    {
        antennaOmnidirectional->antennaHeight
            = ANTENNA_DEFAULT_HEIGHT;
    }
    phyData->antennaData->antennaVar
        = antennaOmnidirectional;
    phyData->antennaData->antennaPatternType
        = ANTENNA_PATTERN_TRADITIONAL;
    phyData->antennaData->numModels++;
    phyData->antennaData->antennaModelType
        = ANTENNA_OMNIDIRECTIONAL;
}

// /**
// FUNCTION :: ANTENNA_InitFromConfigFile
// LAYER :: Physical Layer.
// PURPOSE : Initialize antenna from the default.config file.
// PARAMETERS ::
// + node : Node* : node being initialized.
// + phyIndex : int : interface for which physical to be
//                    initialized.
// + nodeInput : const NodeInput* : structure containing contents of input
//                                  file.
// RETURN :: void : NULL
// **/

void ANTENNA_InitFromConfigFile(
    Node* node,
    int phyIndex,
    const NodeInput* nodeInput)
{
    PhyData* phyData = node->phyData[phyIndex];
    char buf[MAX_STRING_LENGTH];
    BOOL wasFound;
    double antennaEfficiency;
    double antennaMismatchLoss_dB;
    double antennaConnectionLoss_dB;
    double antennaCableLoss_dB;
    double systemLoss_dB = 0.0;


    // The effects of the following 4 variables are currently
    // aggregated and stored in phyData->systemLoss_dB

    // Set ANTENNA-EFFICIENCY
    IO_ReadDouble(
        node,
        node->nodeId,
        phyData->macInterfaceIndex,
        nodeInput,
        "ANTENNA-EFFICIENCY",
        &wasFound,
        &antennaEfficiency);

    if (wasFound)
    {
        systemLoss_dB = -IN_DB(antennaEfficiency);
    }
    else
    {
        systemLoss_dB = -IN_DB(ANTENNA_DEFAULT_EFFICIENCY);
    }


    // Set ANTENNA-MISMATCH-LOSS
    IO_ReadDouble(
        node,
        node->nodeId,
        phyData->macInterfaceIndex,
        nodeInput,
        "ANTENNA-MISMATCH-LOSS",
        &wasFound,
        &antennaMismatchLoss_dB);

    if (wasFound)
    {
        systemLoss_dB += antennaMismatchLoss_dB;
    }
    else
    {
        systemLoss_dB += ANTENNA_DEFAULT_MISMATCH_LOSS_dB;
    }

    // Set ANTENNA-CABLE-LOSS
    IO_ReadDouble(
        node,
        node->nodeId,
        phyData->macInterfaceIndex,
        nodeInput,
        "ANTENNA-CABLE-LOSS",
        &wasFound,
        &antennaCableLoss_dB);

    if (wasFound)
    {
        systemLoss_dB += antennaCableLoss_dB;
    }
    else
    {
        systemLoss_dB += ANTENNA_DEFAULT_CABLE_LOSS_dB;
    }

    // Set ANTENNA-CONNECTION-LOSS
    IO_ReadDouble(
        node,
        node->nodeId,
        phyData->macInterfaceIndex,
        nodeInput,
        "ANTENNA-CONNECTION-LOSS",
        &wasFound,
        &antennaConnectionLoss_dB);

    if (wasFound)
    {
        systemLoss_dB += antennaConnectionLoss_dB;
    }
    else
    {
        systemLoss_dB += ANTENNA_DEFAULT_CONNECTION_LOSS_dB;
    }

    phyData->systemLoss_dB = systemLoss_dB;

    IO_ReadString(
        node,
        node->nodeId,
        phyData->macInterfaceIndex,
        nodeInput,
        "ANTENNA-MODEL",
        &wasFound,
        buf);

    if (!wasFound || strcmp(buf, "OMNIDIRECTIONAL") == 0)
    {
        ANTENNA_OmniDirectionalInitFromConfigFile(node,
            phyIndex, nodeInput);
    }
    else if (strcmp(buf, "SWITCHED-BEAM") == 0)
    {

        ANTENNA_SwitchedBeamInitFromConfigFile(node, phyIndex, nodeInput);
    }
    else if (strcmp(buf, "STEERABLE") == 0)
    {
        ANTENNA_SteerableInitFromConfigFile(node, phyIndex, nodeInput);
    }

    else if (strcmp(buf, "PATTERNED") == 0)
    {
        ANTENNA_PatternedInitFromConfigFile(node, phyIndex, nodeInput);
    }

    else
    {
        char err[MAX_STRING_LENGTH];
        sprintf(err, "Unknown ANTENNA-MODEL %s for phy %d.\n",
            buf, phyIndex);
        ERROR_ReportError(err);
    }

}


// /**
// FUNCTION :: ANTENNA_Init
// LAYER :: Physical Layer.
// PURPOSE :: Initialize antennas.
// PARAMETERS ::
// + node : Node* : node being initialized.
// + phyIndex  : int : interface for which physical to be
//                     initialized.
// + nodeInput : const NodeInput* : structure containing contents of input
//                                  file.
// RETURN :: void : NULL
// **/

void ANTENNA_Init(
     Node* node,
     int phyIndex,
     const NodeInput* nodeInput)
{
    PhyData* phyData = node->phyData[phyIndex];
    char buf[MAX_STRING_LENGTH] = {0};
    char buf1[MAX_STRING_LENGTH] = {0};
    BOOL wasFound;
    AntennaModelGlobal* antennaModel = NULL;
    int i;
    int j;

    int temp;
    // read the antenna orientation

        IO_ReadInt(
            node,
            node->nodeId,
            phyData->macInterfaceIndex,
            nodeInput,
            "ANTENNA-ORIENTATION-AZIMUTH",
            &wasFound,
            &temp);

    if (wasFound)
    {
        (phyData->antennaMountingAngle).azimuth = (OrientationType)temp;
    }
    else
    {
        (phyData->antennaMountingAngle).azimuth = 0;
    }


    IO_ReadInt(
        node,
        node->nodeId,
        phyData->macInterfaceIndex,
        nodeInput,
        "ANTENNA-ORIENTATION-ELEVATION",
        &wasFound,
        &temp);

    if (wasFound)
    {
        (phyData->antennaMountingAngle).elevation = (OrientationType)temp;
    }
    else
    {
        (phyData->antennaMountingAngle).elevation = 0;
    }

    IO_ReadString(
        node,
        node->nodeId,
        phyData->macInterfaceIndex,
        nodeInput,
        "ANTENNA-MODEL",
        &wasFound,
        buf);

    if (!wasFound || (strcmp(buf, "OMNIDIRECTIONAL") == 0)
        || (strcmp(buf, "SWITCHED-BEAM") == 0)
        || (strcmp(buf, "STEERABLE") == 0)
        || (strcmp(buf, "PATTERNED") == 0))
    {
        ANTENNA_InitFromConfigFile(node, phyIndex,nodeInput);
    }
    else
    {
        antennaModel
            = ANTENNA_GlobalAntennaModelGet(node->partitionData,
                                            buf);

        // Create a new global antenna structure
        if (antennaModel == NULL)
        {
            NodeInput* antennaModelInput
                = ANTENNA_MakeAntennaModelInput(nodeInput, buf);

            ERROR_Assert(antennaModelInput ,
                "Unable to form the antennaModelInput.\n");

            IO_ReadString(
                node,
                node->nodeId,
                phyData->macInterfaceIndex,
                antennaModelInput,
                "ANTENNA-MODEL-TYPE",
                &wasFound,
                buf1);
            if (!wasFound)
            {
                ERROR_ReportError(
                 "ANTENNA-MODEL-TYPE is missing for the antenna model.\n");
            }
            else
            {
                ERROR_Assert(strcmp(buf ,buf1) != 0,
                    "antennamodel name and antennaModel class are same.\n");
            }


            ANTENNA_GlobalAntennaModelInit(node,
                phyIndex ,antennaModelInput, buf);

            antennaModel
                = ANTENNA_GlobalAntennaModelGet(
                    node->partitionData, buf);

            for (i = 0; i < antennaModelInput->numFiles; i++)
            {
                MEM_free(antennaModelInput->cachedFilenames[i]);
                MEM_free(antennaModelInput->cached[i]);
            }

            for (j = 0; j < MAX_ANTENNA_NUM_LINES; j++)
            {
                MEM_free(antennaModelInput->inputStrings[j]);
                MEM_free(antennaModelInput->qualifiers[j]);
                MEM_free(antennaModelInput->variableNames[j]);
                MEM_free(antennaModelInput->values[j]);
            }

            MEM_free(antennaModelInput->instanceIds);
            MEM_free(antennaModelInput);

        }
        ERROR_Assert(antennaModel, "Antenna model not found");


        if (antennaModel->antennaModelType == ANTENNA_OMNIDIRECTIONAL)
        {
            ANTENNA_OmniDirectionalInit(node, nodeInput, phyIndex, antennaModel);
            return;
        }
        else if (antennaModel->antennaModelType == ANTENNA_SWITCHED_BEAM)
        {
            ANTENNA_SwitchedBeamInit(node, nodeInput, phyIndex, antennaModel);
            return;
        }
        else if (antennaModel->antennaModelType == ANTENNA_STEERABLE)
        {
            ANTENNA_SteerableInit(node, nodeInput, phyIndex, antennaModel);
            return;
        }
        else if (antennaModel->antennaModelType == ANTENNA_PATTERNED)
        {
            ANTENNA_PatternedInit(node, nodeInput, phyIndex, antennaModel);
            return;
        }
        else
        {
            char err[MAX_STRING_LENGTH];
            sprintf(err, "Unknown ANTENNA-MODEL-TYPE %s.\n", buf);
            ERROR_ReportError(err);
        }

        //If user wants to add some new antennas then he has to put
        //a checking here to initialize the new antenna models.

    }//end of else

}


// /**
// FUNCTION :: ANTENNA_ReadPatterns
// LAYER  :: Physical Layer.
// PURPOSE :: Read in the azimuth pattern file.
// PARAMETERS ::
// + node : Node* : node being used.
// + phyIndex : int : interface for which physical to be
//                    initialized.
// + antennaInput : const NodeInput* : structure containing contents of
//                                     input file.
// + numPatterns  : int* : contains the number of patterns
//                         in the pattern file.
// + steerablePatternSetRepeatSectorAngle : int* :contains
//                                          PatternSetRepeatSectorAngle
//                                          for steerable antenna.
// + pattern_dB : float*** : array used to store the gain values
//                           of the pattern file.
// + azimuthPlane : BOOL : shows whether the file is azimuth
//                         file or elevation file.
// RETURN :: void : NULL
// **/

void ANTENNA_ReadPatterns(
     Node* node,
     int phyIndex,
     const NodeInput* antennaInput,
     int* numPatterns,
     int* steerablePatternSetRepeatAngle,
     float*** pattern_dB,
     BOOL azimuthPlane)
{
    int i;
    char Token[MAX_STRING_LENGTH];
    char* StrPtr;

    int PatternIndex = 0;
    int DegreeIndex = 0;
    int PrevPatternIndex;
    int PrevDegreeIndex;
    float Degree;
    float Gain_dB;

    int NumPatterns = 0;
    float** Pattern_dB = NULL;
    int startLine;


    // Get NumPatterns from the file

    IO_GetToken(Token, antennaInput->inputStrings[0], &StrPtr);
    if (strcmp(Token, "NUMBER-OF-RADIATION-PATTERNS") != 0)
    {
        ERROR_ReportError("Error:NUMBER-OF-RADIATION-PATTERNS is missing\n"
            "at the beginning of antenna pattern file\n");
    }

    ERROR_Assert(StrPtr, "Illegal pattern file.\n");
    IO_GetToken(Token, StrPtr, &StrPtr);
    NumPatterns = (int)atoi(Token);
    ERROR_Assert(NumPatterns > 0,
        "Number of patterns given less than zero.\n");

    startLine = 1;
    *steerablePatternSetRepeatAngle = 360;

    IO_GetToken(Token, antennaInput->inputStrings[startLine], &StrPtr);
    if (strcmp(Token, "STEERABLE-PATTERN-SET-REPEAT-ANGLE") == 0)
    {
        startLine++;
        IO_GetToken(Token, StrPtr, &StrPtr);
        *steerablePatternSetRepeatAngle = (int)atoi(Token);
    }//if//

    ERROR_Assert((*steerablePatternSetRepeatAngle > 0)
        && (*steerablePatternSetRepeatAngle <= 360)
        && (360 % (*steerablePatternSetRepeatAngle) == 0),
        "Illegal steerablePatternSetRepeatAngle.\n");


    // Allocate Pattern_dB array

    Pattern_dB = (float**)MEM_malloc(NumPatterns * sizeof(float*));

    for (i = 0; i < NumPatterns; i++)
    {
        Pattern_dB[i] =
            (float*)MEM_malloc(ANGLE_RESOLUTION * sizeof(float));

        FillArrayValues(
            Pattern_dB[i],
            0,
            ANGLE_RESOLUTION - 1,
            ANTENNA_LOWEST_GAIN_dBi);
    }

    // Read patterns into Pattern_dB array

    PrevPatternIndex = -1;
    PrevDegreeIndex = 0;

    for (i = startLine; i < antennaInput->numLines; i++)
    {

        IO_GetToken(Token, antennaInput->inputStrings[i], &StrPtr);
        PatternIndex = (int)atoi(Token);
        ERROR_Assert((PatternIndex >= 0 && PatternIndex < NumPatterns),
            "Illegal pattern Index in the file.\n");
        ERROR_Assert((PatternIndex == PrevPatternIndex
            || PatternIndex == PrevPatternIndex + 1),
            "Illegal pattern Index in the file.\n");

        ERROR_Assert(StrPtr, "Illegal pattern file.\n");
        IO_GetToken(Token, StrPtr, &StrPtr);
        Degree = (float)atof(Token);
        DegreeIndex = (OrientationType)Degree;

        if (azimuthPlane == FALSE)
        {
            DegreeIndex += ANGLE_RESOLUTION / 2;
        }

        ERROR_Assert(DegreeIndex >= 0 ,"Illegal degree given.\n");
        ERROR_Assert(DegreeIndex < ANGLE_RESOLUTION ,
            "Illegal degree given.\n");

        ERROR_Assert(StrPtr, "Illegal pattern file.\n");
        IO_GetToken(Token, StrPtr, &StrPtr);
        Gain_dB = (float)atof(Token);

        if (PatternIndex != PrevPatternIndex)
        {
            //
            // Each pattern must start with 0 degree
            //
            ERROR_Assert(DegreeIndex == 0 ,
                "Pattern does not start with zero.\n");

            //
            // If the previous pattern setting is unfinished,
            // fill the incomplete fields with the last value
            //
            if (PrevPatternIndex != -1 &&
                PrevDegreeIndex != ANGLE_RESOLUTION - 1)
            {
                FillArrayValues(
                    Pattern_dB[PrevPatternIndex],
                    PrevDegreeIndex + 1,
                    ANGLE_RESOLUTION - 1,
                    Pattern_dB[PrevPatternIndex][PrevDegreeIndex]);
            }
            PrevPatternIndex = PatternIndex;
            PrevDegreeIndex = 0;
        }

        ERROR_Assert(DegreeIndex >= PrevDegreeIndex ,
            "Illegal Degree given.\n");

        // Fill up the discontinuity with the last value

        FillArrayValues(
            Pattern_dB[PatternIndex],
            PrevDegreeIndex + 1,
            DegreeIndex - 1,
            Pattern_dB[PatternIndex][PrevDegreeIndex]);

        Pattern_dB[PatternIndex][DegreeIndex] = Gain_dB;

        PrevDegreeIndex = DegreeIndex;
    }

    ERROR_Assert(PatternIndex == NumPatterns - 1,
        "Illegal number of patterns in the file.\n");

    if (DegreeIndex != ANGLE_RESOLUTION - 1)
    {
        std::string errorStr;
        errorStr = "Angle resolution is not completely filled out in ";
        errorStr += antennaInput->ourName;
        errorStr += "\nQualnet will fill the antenna pattern with its own"
                    " interpolated data\n";
        ERROR_ReportWarning(errorStr.c_str());
        FillArrayValues(
            Pattern_dB[PatternIndex],
            DegreeIndex + 1,
            ANGLE_RESOLUTION - 1,
            Pattern_dB[PatternIndex][DegreeIndex]);
    }

    *numPatterns = NumPatterns;
    *pattern_dB = Pattern_dB;

    return;
}


// /**
// FUNCTION :: ANTENNA_ReadNsmaPatterns
// LAYER :: Physical Layer
// PURPOSE :: Read in the NSMA pattern file.
// PARAMETERS ::
// + node : Node*  : node being used.
// + phyIndex : int : interface for which physical
//                    to be initialized.
// +antennaInput : NodeInput* : structure containing contents of
//                              input file.
// +numPatterns : int : number of patterns in the file.
// +azimuthPattern_dB : float*** : pattern_dB array for
//                                 azimuth gains.
// +azimuthResolution : int* : azimuth resolution
//                     and azimuth range.
// +elevationPattern_dB : float*** : pattern_dB array
//                                   for elevation gains.
// +elevationResolution : int* : elevation resolution
//                     and elevation range.
// +NSMAPatternVersion* :  version version of NSMA pattern
// RETURN :: void : NULL
// **/

void ANTENNA_ReadNsmaPatterns(
    Node* node,
    int phyIndex,
    const NodeInput* antennaInput,
    int numPatterns,
    float*** azimuthPattern_dB,
    int* azimuthResolution,
    float*** elevationPattern_dB,
    int* elevationResolution,
    NSMAPatternVersion* version)
{
    int i;
    int j;
    int k;
    float aziRatio = (float)((*azimuthResolution)/360.0);
    float elvRatio;
    const char* delims = ",: \n";
    char* StrPtr;

    char Token[MAX_STRING_LENGTH];
    char prevToken[MAX_STRING_LENGTH];
    char elvToken[MAX_STRING_LENGTH];

    IO_GetDelimitedToken(Token,
                         antennaInput->inputStrings[0],
                         delims,
                         &StrPtr);
     if (!strcmp(Token,"REVNUM"))
     {
         *version = NSMA_REVISED;
         if (*elevationResolution < 0)
         {
             *elevationResolution = 360;
         }
         elvRatio = (float)((*elevationResolution)/360.0);

         ANTENNA_ReadRevisedNsmaPatterns(node,
                                         phyIndex,
                                         antennaInput,
                                         numPatterns,
                                         azimuthPattern_dB,
                                         aziRatio,
                                         elevationPattern_dB,
                                         elvRatio);
         return;
     }

    int aziPatternIndex = -1;
    int elvPatternIndex = -1;
    int aziDegreeIndex = 0;
    int elvDegreeIndex = 0;
    int PrevAziPatternIndex = -1;
    int PrevElvPatternIndex = -1;
    int PrevAziDegreeIndex = 0;
    int PrevElvDegreeIndex = 0;
    int firstAziOccur = 0;
    int firstElvOccur = 0;

    float Degree;
    float Gain_dB;

    int dataCount;
    int elvationIndex;

    float** aziPattern_dB = NULL;
    float** elvPattern_dB = NULL;
    float temp;

    BOOL elevationPlane = FALSE;

    *version = NSMA_TRADITIONAL;

    if (*elevationResolution < 0)
     {
         *elevationResolution = 180;
     }

    elvRatio = (float)((*elevationResolution)/180.0);
    // Allocate aziPattern_dB array & fill by lowest gain value
    aziPattern_dB
        = (float**)MEM_malloc(numPatterns * sizeof(float*));

    for (i = 0; i < numPatterns; i++)
    {
        aziPattern_dB[i]
            = (float*)MEM_malloc
                (((*azimuthResolution) + 1) * sizeof(float));

        for (k = 0; k <= (int)(*azimuthResolution); k++)
        {
            aziPattern_dB[i][k] = (-1) * ANTENNA_LOWEST_GAIN_dBi;
        }
    }


    // Allocate elvPattern_dB array & fill by lowest gain value
    elvPattern_dB
        = (float**)MEM_malloc(numPatterns * sizeof(float*));

    for (i = 0; i < numPatterns; i++)
    {
        elvPattern_dB[i]
            = (float*)MEM_malloc
            (((*elevationResolution) + 1) * sizeof(float));

        for (k = 0; k <= (*elevationResolution); k++)
        {
            elvPattern_dB[i][k] = (-1) * ANTENNA_LOWEST_GAIN_dBi;
        }
    }

    int startLine = NSMA_PATTERN_START_LINE_NUMBER;

    IO_GetToken(Token, antennaInput->inputStrings[startLine], &StrPtr);

    if ((strcmp(Token,"HH") != 0) && (strcmp(Token,"HV") != 0)
        && (strcmp(Token,"VV") != 0) && (strcmp(Token,"VH") != 0)
        && (strcmp(Token,"ELHH") != 0) && (strcmp(Token,"ELHV") != 0)
        && (strcmp(Token,"ELVV") != 0) && (strcmp(Token,"ELVH") != 0))
    {
        ERROR_ReportError("Error: NSMA file format is not correct\n");
    }

    for (; startLine < antennaInput->numLines;)
    {
        IO_GetToken(Token, antennaInput->inputStrings[startLine], &StrPtr);

        if ((strcmp(Token,"HH") == 0) || (strcmp(Token,"VV") == 0))
        {

            firstAziOccur++;

            if (firstAziOccur != 1)
            {
                startLine ++;
                continue;
            }

            strcpy(prevToken, Token);

            ERROR_Assert(StrPtr, "Illegal pattern file.\n");
            IO_GetToken(Token, StrPtr, &StrPtr);
            dataCount = (int)atoi(Token);
            ERROR_Assert(dataCount >= 0 ,
                "Illegal number of data in the file.\n");
            aziPatternIndex ++;

            for (j = 0; j < dataCount; j++)
            {
                IO_GetToken(Token, antennaInput->inputStrings[
                    startLine + 1], &StrPtr);
                Degree = (float)atof(Token);

                aziDegreeIndex = (int)(( Degree
                    + (ANGLE_RESOLUTION / 2)) * aziRatio);

                ERROR_Assert(aziDegreeIndex >= 0 ,
                    "Illegal degree given.\n");
                ERROR_Assert(aziDegreeIndex <= (int)(ANGLE_RESOLUTION
                    * aziRatio), "Illegal degree given.\n");

                ERROR_Assert(StrPtr, "Illegal pattern file.\n");
                IO_GetToken(Token, StrPtr, &StrPtr);
                Gain_dB = (float)atof(Token);
                Gain_dB *= -1;

                if (aziPatternIndex != PrevAziPatternIndex)
                {
                    PrevAziPatternIndex = aziPatternIndex;
                    PrevAziDegreeIndex = 0;
                }

                ERROR_Assert(aziDegreeIndex >= PrevAziDegreeIndex,
                    "Illegal degree given.\n");

                // Fill up the discontinuity with the linear
                //interpolation

                ANTENNA_FillGainValues(
                    aziPattern_dB[aziPatternIndex],
                    PrevAziDegreeIndex + 1,
                    aziDegreeIndex - 1,
                    aziPattern_dB[aziPatternIndex]
                    [PrevAziDegreeIndex], Gain_dB);

                aziPattern_dB[aziPatternIndex][aziDegreeIndex]
                    = Gain_dB;

                PrevAziDegreeIndex = aziDegreeIndex;

                startLine ++;
            }//end of for
            for (i=0; i < (int)(ANGLE_RESOLUTION/2 * aziRatio) ;i++)
            {
                temp = aziPattern_dB[aziPatternIndex][i];
                aziPattern_dB[aziPatternIndex][i]
                    = aziPattern_dB[aziPatternIndex]
                        [(int)(i+(ANGLE_RESOLUTION/2 * aziRatio))];
                aziPattern_dB[aziPatternIndex]
                [(int)(i+(ANGLE_RESOLUTION/2 * aziRatio))] = temp;
            }
        }//end of if

        else if ((strcmp(Token,"ELHH") == 0)
                    || (strcmp(Token,"ELVV") == 0))
        {
            strcpy(elvToken,"EL");
            if (strcmp((strcat(elvToken,prevToken)), Token) != 0)
            {
                startLine ++;
                continue;
            }

            firstElvOccur++;

            if (firstElvOccur != 1)
            {
                break;
            }

            IO_GetToken(Token, StrPtr, &StrPtr);
            dataCount = (int)atoi(Token);
            if (dataCount == 0 || strcmp(Token,"NONE") == 0)
            {
                break;
            }

            elevationPlane = TRUE;

            ERROR_Assert(dataCount > 0,
                "Illegal number of data in the file.\n");

            elvPatternIndex ++;
            elvationIndex = 1;

            for (j = 0; j < dataCount; j++)
            {
                IO_GetToken(Token, antennaInput->inputStrings
                    [startLine + 1], &StrPtr);
                Degree = (float)atof(Token);
                elvDegreeIndex = (OrientationType)Degree;

                if (j == 0 && elvDegreeIndex == -5)
                {
                    elvationIndex = 18;
                }

                elvDegreeIndex *= elvationIndex;
                elvDegreeIndex = (int)((elvDegreeIndex
                    + (ANGLE_RESOLUTION / 4)) * elvRatio);

                ERROR_Assert(elvDegreeIndex >= 0 ,
                    "Illegal elevation degree.\n");
                ERROR_Assert(elvDegreeIndex <= ((int)(
                    ANGLE_RESOLUTION * elvRatio)/2) ,
                    "Illegal elevation degree.\n");

                IO_GetToken(Token, StrPtr, &StrPtr);
                Gain_dB = (float)atof(Token);
                Gain_dB *= -1;

                if (elvPatternIndex != PrevElvPatternIndex)
                {
                    PrevElvPatternIndex = elvPatternIndex;
                    PrevElvDegreeIndex = 0;
                }

                ERROR_Assert(elvDegreeIndex >= PrevElvDegreeIndex,
                    "Illegal elevation degree.\n");

                // Fill up the discontinuity with the linear
                // interpolation
                ANTENNA_FillGainValues(
                    elvPattern_dB[elvPatternIndex],
                    PrevElvDegreeIndex + 1,
                    elvDegreeIndex - 1,
                    elvPattern_dB[elvPatternIndex][
                    PrevElvDegreeIndex], Gain_dB);


                elvPattern_dB[elvPatternIndex]
                    [elvDegreeIndex] = Gain_dB;

                PrevElvDegreeIndex = elvDegreeIndex;

                startLine ++;
            }//end of elevation read

        }//end of else if
        startLine ++;
    }//end of for


    if (numPatterns != (aziPatternIndex + 1))
    {
        ERROR_ReportError("Error: NUM-PATTERNS are not correct\n");
    }

    if (elevationPlane == TRUE && numPatterns != (elvPatternIndex + 1))
    {
        ERROR_ReportError("Error: NUM-PATTERNS are not correct\n");
    }
    if (!elevationPlane)
    {
        for (i = 0; i < numPatterns; i++)
        {
            MEM_free(elvPattern_dB[i]);
        }
        MEM_free(elvPattern_dB);
        elvPattern_dB = NULL;
}

    *azimuthPattern_dB = aziPattern_dB;
    *elevationPattern_dB = elvPattern_dB;

}//end of function

// /**
// FUNCTION :: ANTENNA_ReadRevisedNsmaPatterns
// LAYER :: Physical Layer
// PURPOSE :: Read in the Revised NSMA pattern file.
// PARAMETERS ::
// + node : Node*  : node being used.
// + phyIndex : int : interface for which physical
//                    to be initialized.
// +antennaInput : NodeInput* : structure containing contents of
//                              input file.
// +numPatterns : int : number of patterns in the file.
// +azimuthPattern_dB : float*** : pattern_dB array for
//                                 azimuth gains.
// +aziRatio : float : the ratio of azimuth resolution
//                     and azimuth range.
// +elevationPattern_dB : float*** : pattern_dB array
//                                   for elevation gains.
// +elvRatio : float : the ratio of elevation resolution
//                     and elevation range.
// RETURN :: void : NULL
// **/

void ANTENNA_ReadRevisedNsmaPatterns(
    Node* node,
    int phyIndex,
    const NodeInput* antennaInput,
    int numPatterns,
    float*** azimuthPattern_dB,
    float aziRatio,
    float*** elevationPattern_dB,
    float elvRatio)
{
    int i;
    int j;
    int k;
    bool isFormatCorrect = FALSE;

    char Token[MAX_STRING_LENGTH];
    const char* delims = ",: \n";
    char* StrPtr;
    char polarization[MAX_STRING_LENGTH];
    char patCut[MAX_STRING_LENGTH];
    char firstPolarization[MAX_STRING_LENGTH] = "Not Found";

    int aziPatternIndex = -1;
    int elvPatternIndex = -1;
    int aziDegreeIndex = 0;
    int elvDegreeIndex = 0;
    int PrevAziPatternIndex = -1;
    int PrevElvPatternIndex = -1;
    int PrevAziDegreeIndex = 0;
    int PrevElvDegreeIndex = 0;
    int firstAziOccur = 0;
    int firstElvOccur = 0;

    float Degree;
    float Gain_dB;
    float temp;
    BOOL elevationPlane = FALSE;

    int dataCount;
    int elvationIndex;

    float** aziPattern_dB = NULL;
    float** elvPattern_dB = NULL;


    // Allocate aziPattern_dB array & fill by lowest gain value
    aziPattern_dB
        = (float**)MEM_malloc(numPatterns * sizeof(float*));

    for (i = 0; i < numPatterns; i++)
    {
        aziPattern_dB[i]
            = (float*)MEM_malloc
                (((int)(ANGLE_RESOLUTION * aziRatio) + 1) * sizeof(float));

        for (k = 0; k <= (int)(ANGLE_RESOLUTION * aziRatio); k++)
        {
            aziPattern_dB[i][k] = (-1) * ANTENNA_LOWEST_GAIN_dBi;
        }
    }


    // Allocate elvPattern_dB array & fill by lowest gain value
    elvPattern_dB
        = (float**)MEM_malloc(numPatterns * sizeof(float*));

    for (i = 0; i < numPatterns; i++)
    {
        elvPattern_dB[i]
            = (float*)MEM_malloc
                (((int)(ANGLE_RESOLUTION * elvRatio) + 1) * sizeof(float));

        for (k = 0; k <= (int)(ANGLE_RESOLUTION * elvRatio); k++)
        {
            elvPattern_dB[i][k] = (-1) * ANTENNA_LOWEST_GAIN_dBi;
        }
    }

    int startLine = NSMA_PATTERN_START_LINE_NUMBER;

    for (; startLine < NSMA_MAX_STARTLINE; startLine++)
    {
        IO_GetDelimitedToken(Token, antennaInput->inputStrings[startLine],
                             delims, &StrPtr);
        if ((strcmp(Token,"NOFREQ") != 0))
        {
            continue;
        }
        isFormatCorrect = TRUE;
        IO_GetDelimitedToken(Token, StrPtr, delims, &StrPtr);
        if (atoi(Token) > 1)
        {
            ERROR_ReportWarning("Only one freq is supported"
                                " First one will be used ");
        }
        startLine+=3;
        break;
    }

    ERROR_Assert (isFormatCorrect,"Format specified is not correct");

    for (; startLine < antennaInput->numLines-1;)
    {
        IO_GetDelimitedToken(Token, antennaInput->inputStrings[startLine],
                             delims, &StrPtr);
        if (!strcmp(Token,"PATCUT"))
        {
            IO_GetDelimitedToken(patCut, StrPtr, delims, &StrPtr);
            IO_GetDelimitedToken(Token, antennaInput->
                                inputStrings[++startLine],delims, &StrPtr);
            if ((strcmp(Token,"POLARI") != 0))
    {
        ERROR_ReportError("Error: NSMA file format is not correct\n");
    }
            IO_GetDelimitedToken(polarization, StrPtr, delims, &StrPtr);
            IO_GetDelimitedToken(Token, antennaInput->
                                inputStrings[++startLine],delims, &StrPtr);
            if ((strcmp(Token,"NUPOIN") != 0))
    {
                ERROR_ReportError("Error: NSMA file format is not correct\n");
            }
            IO_GetDelimitedToken(Token, StrPtr, delims, &StrPtr);
            dataCount = (int)atoi(Token);
            ERROR_Assert(dataCount >= 0 ,
                "Illegal number of data in the file.\n");
            startLine ++;
            IO_GetDelimitedToken(Token,
                                antennaInput->inputStrings[startLine + 1],
                                delims, &StrPtr);
            while ((strcmp(Token,"XORIEN") == 0) ||
                  (strcmp(Token,"YORIEN") == 0) ||
                  (strcmp(Token,"ZORIEN") == 0))
        {
                startLine++;
                IO_GetDelimitedToken(Token,
                                    antennaInput->inputStrings[startLine + 1],
                                    delims, &StrPtr);

            }

            if ((strcmp(polarization,"H/H") == 0) ||
                (strcmp(polarization,"V/V") == 0))
            {
                if ((strcmp(patCut,"AZ") == 0) || (strcmp(patCut,"H") == 0))
                {
                    firstAziOccur++;
            if (firstAziOccur != 1)
            {
                startLine ++;
                continue;
            }
                    if (!strcmp(firstPolarization,"Not Found"))
                    {
                        strcpy(firstPolarization, polarization);
                    }
                    else if (strcmp(firstPolarization, polarization) != 0)
                    {
                        startLine ++;
                        continue;
                    }
            aziPatternIndex ++;

            for (j = 0; j < dataCount; j++)
            {
                        IO_GetDelimitedToken(Token, antennaInput->
                            inputStrings[startLine + 1],delims, &StrPtr);
                Degree = (float)atof(Token);

                        aziDegreeIndex = (int)(( Degree
                    + (ANGLE_RESOLUTION / 2)) * aziRatio);

                ERROR_Assert(aziDegreeIndex >= 0 ,
                    "Illegal degree given.\n");
                ERROR_Assert(aziDegreeIndex <= (int)(ANGLE_RESOLUTION
                    * aziRatio), "Illegal degree given.\n");

                        IO_GetDelimitedToken(Token, StrPtr, delims, &StrPtr);
                Gain_dB = (float)atof(Token);
                Gain_dB *= -1;

                if (aziPatternIndex != PrevAziPatternIndex)
                {
                    PrevAziPatternIndex = aziPatternIndex;
                    PrevAziDegreeIndex = 0;
                }

                ERROR_Assert(aziDegreeIndex >= PrevAziDegreeIndex,
                    "Illegal degree given.\n");

                // Fill up the discontinuity with the linear
                //interpolation

                ANTENNA_FillGainValues(
                    aziPattern_dB[aziPatternIndex],
                    PrevAziDegreeIndex + 1,
                    aziDegreeIndex - 1,
                    aziPattern_dB[aziPatternIndex]
                    [PrevAziDegreeIndex], Gain_dB);

                aziPattern_dB[aziPatternIndex][aziDegreeIndex]
                    = Gain_dB;

                PrevAziDegreeIndex = aziDegreeIndex;

                startLine ++;
            }//end of for
                    for (i=0; i < (int)(ANGLE_RESOLUTION/2 * aziRatio) ;i++)
                    {
                        temp = aziPattern_dB[aziPatternIndex][i];
                        aziPattern_dB[aziPatternIndex][i]
                            = aziPattern_dB[aziPatternIndex]
                                [(int)(i+(ANGLE_RESOLUTION/2 * aziRatio))];
                        aziPattern_dB[aziPatternIndex]
                        [(int)(i+(ANGLE_RESOLUTION/2 * aziRatio))] = temp;
                    }
        }//end of if

                else if ((strcmp(patCut,"EL") == 0)
                        || (strcmp(patCut,"V") == 0))
        {
                    if (!strcmp(firstPolarization,"Not Found"))
                    {
                        strcpy(firstPolarization, polarization);
                    }
                    else if (strcmp(firstPolarization, polarization) != 0)
            {
                startLine ++;
                continue;
            }

            firstElvOccur++;

            if (firstElvOccur != 1)
            {
                break;
            }

                    elevationPlane = TRUE;

            elvPatternIndex ++;
            elvationIndex = 1;

            for (j = 0; j < dataCount; j++)
            {
                        IO_GetDelimitedToken(Token, antennaInput->inputStrings
                            [startLine + 1],delims, &StrPtr);
                Degree = (float)atof(Token);
                elvDegreeIndex = (OrientationType)Degree;

                        if (j == 0 && elvDegreeIndex == -10)
                {
                    elvationIndex = 18;
                }

                elvDegreeIndex *= elvationIndex;
                elvDegreeIndex = (int)((elvDegreeIndex
                            + (ANGLE_RESOLUTION / 2)) * elvRatio);

                ERROR_Assert(elvDegreeIndex >= 0 ,
                    "Illegal elevation degree.\n");
                ERROR_Assert(elvDegreeIndex <= ((int)(
                            ANGLE_RESOLUTION * elvRatio)) ,
                    "Illegal elevation degree.\n");

                        IO_GetDelimitedToken(Token, StrPtr, delims, &StrPtr);
                Gain_dB = (float)atof(Token);
                Gain_dB *= -1;

                if (elvPatternIndex != PrevElvPatternIndex)
                {
                    PrevElvPatternIndex = elvPatternIndex;
                    PrevElvDegreeIndex = 0;
                }

                ERROR_Assert(elvDegreeIndex >= PrevElvDegreeIndex,
                    "Illegal elevation degree.\n");

                // Fill up the discontinuity with the linear
                // interpolation
                ANTENNA_FillGainValues(
                    elvPattern_dB[elvPatternIndex],
                    PrevElvDegreeIndex + 1,
                    elvDegreeIndex - 1,
                    elvPattern_dB[elvPatternIndex][
                    PrevElvDegreeIndex], Gain_dB);


                elvPattern_dB[elvPatternIndex]
                    [elvDegreeIndex] = Gain_dB;

                PrevElvDegreeIndex = elvDegreeIndex;

                startLine ++;
            }//end of elevation read

        }//end of else if
            }
        }
        else if (!strcmp(Token,"PATFRE") || !strcmp(Token,"ENDFIL"))
        {
            break;
        }
        startLine ++;

    }//end of for


    if (numPatterns != (aziPatternIndex + 1))
    {
        ERROR_ReportError("Error: NUM-PATTERNS are not correct\n");
    }

    if (elevationPlane == TRUE && numPatterns != (elvPatternIndex + 1))
    {
        ERROR_ReportError("Error: NUM-PATTERNS are not correct\n");
    }

    if (!elevationPlane)
    {
        for (i = 0; i < numPatterns; i++)
        {
            MEM_free(elvPattern_dB[i]);
        }
        MEM_free(elvPattern_dB);
        elvPattern_dB = NULL;
    }

    *azimuthPattern_dB = aziPattern_dB;
    *elevationPattern_dB = elvPattern_dB;

}//end of function

// /**
// FUNCTION :: ANTENNA_ReturnTraditionalPatternFile
// LAYER :: Physical Layer.
// PURPOSE :: Used to read Qualnet Traditional pattern file
// PARAMETERS ::
// + node : Node* : node being used
// + phyIndex : int : interface for which
//                    physical to be initialized
// + antennaModelInput : const NodeInput* : structure containing
//                                          contents of input
//                                          file
// +antennaPatterns : AntennaPatterns* : Pointer to
//                                       the global antenna
//                                       pattern structure.
// RETURN :: void : NULL
// /**

void ANTENNA_ReturnTraditionalPatternFile(
     Node* node,
     int  phyIndex,
     const NodeInput*  antennaModelInput,
     AntennaPattern* antennaPatterns)
{

    PhyData* phyData = node->phyData[phyIndex];

    BOOL wasFound;
    int numPatterns;
    int notUsed;
    int i, j;
    double highestGain;
    int    highestGainAngle;
    NodeInput antennaInput;
    char err[MAX_STRING_LENGTH];

    float** patternAzimuth_dBi;
    float** patternElevation_dB;
    AntennaPatternElement** antennaPatternElements;

    // Azimuth plane initialization with
    // ANTENNA-AZIMUTH-PATTERN-FILE (mandatory)
    // ANTENNA-ELEVATION-PATTERN-FILE is optional

    IO_ReadCachedFile(
        node,
        node->nodeId,
        phyData->macInterfaceIndex,
        antennaModelInput,
        "ANTENNA-AZIMUTH-PATTERN-FILE",
        &wasFound,
        &antennaInput);

    if (!wasFound)
    {
        sprintf(err,
            "Error: ANTENNA-AZIMUTH-PATTERN-FILE "
            "is missing for the node %d\n",
            node->nodeId);
        ERROR_ReportError(err);
    }

    strcpy(antennaPatterns->antennaFileName, antennaInput.ourName);


    antennaPatterns->azimuthUnits =
        ANTENNA_PATTERN_UNITS_DEGREES_0_TO_360;
    antennaPatterns->elevationUnits =
        ANTENNA_PATTERN_UNITS_DEGREES_MINUS_180_TO_180;
    antennaPatterns->azimuthResolution = 360;
    antennaPatterns->elevationResolution = 360;
    antennaPatterns->maxAzimuthIndex = 359;
    antennaPatterns->maxElevationIndex = 179;
    antennaPatterns->is3DArray = FALSE;
    antennaPatterns->is3DGeometry = FALSE;

    ANTENNA_ReadPatterns(
        node,
        phyIndex,
        &antennaInput,
        &numPatterns,
        &notUsed,
        &patternAzimuth_dBi,
        TRUE);

    antennaPatterns->patternSetRepeatAngle = notUsed;

    antennaPatterns->numOfPatterns = numPatterns;

    antennaPatternElements
        = (AntennaPatternElement**)
            MEM_malloc(AZIMUTH_ELEVATION_INDEX
                * sizeof(AntennaPatternElement*));

    antennaPatternElements[AZIMUTH_INDEX]
        = (AntennaPatternElement*)
        MEM_malloc(numPatterns * sizeof(AntennaPatternElement));

    antennaPatternElements[ELEVATION_INDEX]
        = (AntennaPatternElement*)
        MEM_malloc(numPatterns * sizeof(AntennaPatternElement));

    antennaPatterns->boreSightAngle
        = (Orientation *)
        MEM_malloc(numPatterns * sizeof(Orientation));
    antennaPatterns->boreSightGain_dBi =
        (float *)MEM_malloc(numPatterns * sizeof(float));

    for (i = 0; i < numPatterns; i++)
    {
        highestGain = ANTENNA_LOWEST_GAIN_dBi;
        highestGainAngle = 0;

        antennaPatternElements[AZIMUTH_INDEX][i].units
            = ANTENNA_GAIN_UNITS_DBI;
        antennaPatternElements[AZIMUTH_INDEX][i].numGainValues
            = ANGLE_RESOLUTION;
        antennaPatternElements[AZIMUTH_INDEX][i].gains
            = (AntennaElementGain*) MEM_malloc(
            ANGLE_RESOLUTION * sizeof(AntennaElementGain));

        for (j = 0; j < ANGLE_RESOLUTION; j++)
        {
            ERROR_Assert(patternAzimuth_dBi[i][j]
                != ANTENNA_LOWEST_GAIN_dBi,
                "gain value is ANTENNA_LOWEST_GAIN_dBi.\n");

            antennaPatternElements[AZIMUTH_INDEX][i].gains[j]
                = patternAzimuth_dBi[i][j];

            if (DEBUG_TRADITIONAL)
            {

                printf("patternIndex %d\tangle %d\tgain%f\n",i,j,
                    antennaPatternElements[AZIMUTH_INDEX][i].gains[j]);
            }

            if (patternAzimuth_dBi[i][j] > highestGain)
            {
                highestGain = patternAzimuth_dBi[i][j];
                highestGainAngle = j;
            }
        }
        ERROR_Assert(highestGain != ANTENNA_LOWEST_GAIN_dBi ,
            "highest gain is ANTENNA_LOWEST_GAIN_dBi.\n");

        antennaPatterns->boreSightAngle[i].azimuth
            = (short) highestGainAngle;
        antennaPatterns->boreSightGain_dBi[i] = (float) highestGain;
        MEM_free(patternAzimuth_dBi[i]);
    }
    MEM_free(patternAzimuth_dBi);

    // Elevation plane initialization with
    // ANTENNA-ELEVATION-PATTERN-FILE is optional
    IO_ReadCachedFile(
        node,
        node->nodeId,
        phyData->macInterfaceIndex,
        antennaModelInput,
        "ANTENNA-ELEVATION-PATTERN-FILE",
        &wasFound,
        &antennaInput);

    if (!wasFound)
    {
        for (i = 0; i < numPatterns; i++)
        {
            antennaPatterns->boreSightAngle[i].elevation = 0;
        }

        MEM_free(antennaPatternElements[ELEVATION_INDEX]);
        antennaPatternElements[ELEVATION_INDEX] = NULL;
        antennaPatterns->antennaPatternElements =
            antennaPatternElements;

        return;
    }

    ANTENNA_ReadPatterns(
        node,
        phyIndex,
        &antennaInput,
        &numPatterns,
        &notUsed,
        &patternElevation_dB,
        FALSE);

    ERROR_Assert(antennaPatterns->numOfPatterns == numPatterns,
        "mismatch in number of patterns between"
        "azimuth and elevation files.\n");

    for (i = 0; i < numPatterns; i++)
    {
        highestGain = ANTENNA_LOWEST_GAIN_dBi;
        highestGainAngle = 0;

        antennaPatternElements[ELEVATION_INDEX][i].units
            = ANTENNA_GAIN_UNITS_DBI;
        antennaPatternElements[ELEVATION_INDEX][i].numGainValues
            = ANGLE_RESOLUTION;
        antennaPatternElements[ELEVATION_INDEX][i].gains
            = (AntennaElementGain*) MEM_malloc(
            ANGLE_RESOLUTION * sizeof(AntennaElementGain));

        for (j = 0; j < ANGLE_RESOLUTION; j++)
        {
            ERROR_Assert(patternElevation_dB[i][j]
                != ANTENNA_LOWEST_GAIN_dBi,
                "gain is ANTENNA_LOWEST_GAIN_dBi.\n");

            antennaPatternElements[ELEVATION_INDEX][i].gains[j]
                = patternElevation_dB[i][j];

            if (DEBUG_TRADITIONAL)
            {
                printf("patternIndex %d\tangle %d\tgain%f\n",i,j-180,
                    antennaPatternElements[ELEVATION_INDEX][i].gains[j]);
            }

            if (patternElevation_dB[i][j] > highestGain)
            {
                highestGain = patternElevation_dB[i][j];
                highestGainAngle = j;
            }
        }
        ERROR_Assert(highestGain != ANTENNA_LOWEST_GAIN_dBi,
            "highest gain is ANTENNA_LOWEST_GAIN_dBi.\n");

        antennaPatterns->boreSightAngle[i].elevation = (short)
            (highestGainAngle - ANGLE_RESOLUTION / 2);
        antennaPatterns->boreSightGain_dBi[i] += (float)highestGain;
        MEM_free(patternElevation_dB[i]);
    }
    MEM_free(patternElevation_dB);

    antennaPatterns->antennaPatternElements = antennaPatternElements;

}



// /**
// FUNCTION :: ANTENNA_Read3DAsciiPatterns
// LAYER :: Physical Layer.
// PURPOSE :: Used to read ASCII 3D pattern file.
// PARAMETERS ::
// + node : Node* : node being used.
// + phyIndex : int : interface for which physical
//                    to be initialized.
// +antennaInput : NodeInput* : structure containing contents of
//                              input file.
// +antennaPatterns : AntennaPattern* : Pointer to Global Antenna Pattern
//                                      structure.
// RETURN :: void : NULL
// **/

void ANTENNA_Read3DAsciiPatterns(
     Node* node,
     int phyIndex,
     const NodeInput* antennaInput,
     AntennaPattern* antennaPatterns)

{
    int i;
    int j;
    int k;
    char Token[MAX_STRING_LENGTH];
    char err[MAX_STRING_LENGTH];
    char* StrPtr;
    float prevAzimuthDegree = 0;
    float azimuthDegree = 0;
    float prevElevationDegree = 0;
    float elevationDegree;
    float Gain_dB;
    float PrevGain_dB = 1.0f;
    float ratio;
    float ratio1;
    int  aziIndex = 0;//angleResolution
    int eleIndex = -1;//numPatterns
    int patternIndex = -1;

    BOOL flag = FALSE;
    BOOL flag1 = FALSE;
    BOOL flag2 = FALSE;
    ratio = (float) (antennaPatterns->azimuthResolution) / 360;
    ratio1 = (float) (antennaPatterns->elevationResolution) / 180;
    float delta;

    float*** image = NULL;
    image = (float***)MEM_malloc(antennaPatterns->
        numOfPatterns * sizeof(float**));

    for (i = 0;i < antennaPatterns->numOfPatterns; i++)
    {
        image[i] = (float**)MEM_malloc((antennaPatterns->
            elevationResolution + 1) * sizeof(float*));
    }

    for (i = 0;i < antennaPatterns->numOfPatterns;i++)
    {
        for (j = 0;j <= antennaPatterns->elevationResolution; j++)
        {
            image[i][j] = (float*)MEM_malloc((antennaPatterns->
                azimuthResolution + 1) * sizeof(float));
        }
    }

    for (i = 0; i < antennaPatterns->numOfPatterns; i++)
    {
        for (j = 0;j <= antennaPatterns->elevationResolution; j++)
        {
            for (k = 0; k <= antennaPatterns->azimuthResolution; k++)
            {
                image[i][j][k] = (-1) * ANTENNA_LOWEST_GAIN_dBi;
            }
        }
    }


    for (j = 0; j < antennaInput->numLines; j++)
    {

        ERROR_Assert(antennaInput->inputStrings[j], "Illegal pattern file.\n");
        IO_GetToken(Token, antennaInput->inputStrings[j], &StrPtr);

        elevationDegree = (float)atof(Token);

        ERROR_Assert(StrPtr, "Illegal pattern file.\n");
        IO_GetToken(Token, StrPtr, &StrPtr);

        azimuthDegree = (float)atof(Token);

        ERROR_Assert(StrPtr, "Illegal pattern file.\n");
        IO_GetToken(Token, StrPtr, &StrPtr);

        Gain_dB = (float)atof(Token);
        Gain_dB *= -1;

        if (elevationDegree == 0 && azimuthDegree == 0)
        {
            flag = FALSE;
            patternIndex++;
            ERROR_Assert(patternIndex < antennaPatterns->numOfPatterns,
                        "Illegal number of patterns in the file.\n");

        if ((aziIndex != antennaPatterns->azimuthResolution) &&
            patternIndex !=0)
        {
            while (aziIndex < antennaPatterns->azimuthResolution)
            {
                ++aziIndex;
                image[patternIndex-1][eleIndex][aziIndex]
                        = PrevGain_dB;
            }
        }

            if ((eleIndex != (antennaPatterns->elevationResolution))
                && eleIndex >= 0)
            {
                eleIndex++;
                while (eleIndex <= antennaPatterns->elevationResolution)
                {
                    for (aziIndex = 0; aziIndex <= antennaPatterns->
                        azimuthResolution ; aziIndex++)
                    {
                        image[patternIndex - 1][eleIndex][aziIndex]
                            = image[patternIndex -1][eleIndex - 1][aziIndex];
                    }
                    eleIndex++;
                }
            }
            aziIndex = 0;
            eleIndex = -1;

        }

        if ((aziIndex != antennaPatterns->azimuthResolution) &&
            (azimuthDegree == 0) && (elevationDegree > prevElevationDegree))
        {
            while (aziIndex < antennaPatterns->azimuthResolution)
            {
                ++aziIndex;
                image[patternIndex][eleIndex][aziIndex]
                        = PrevGain_dB;
            }
            flag = FALSE;
        }

        if (aziIndex == antennaPatterns->azimuthResolution)
        {
            flag = FALSE;
        }

        if (flag == FALSE)  //the case for 0 azimuth degree
        {
            ERROR_Assert(azimuthDegree == 0 ,
                "Illegal azimuth degree.\n");
            if (elevationDegree > prevElevationDegree)
            {
                for (i = 1; i < ((elevationDegree - prevElevationDegree)
                                * ratio1); i++)
                {
                    for (aziIndex = 0; aziIndex <= antennaPatterns->
                        azimuthResolution; aziIndex++)
                    {
                        image[patternIndex][eleIndex + i][aziIndex]
                          = image[patternIndex][eleIndex + i - 1][aziIndex];
                    }

                    flag1 = TRUE;
                }

                if (flag1 == TRUE)
                {
                    eleIndex = eleIndex + i - 1;
                }
                prevElevationDegree = elevationDegree;
                prevAzimuthDegree = azimuthDegree;
                PrevGain_dB = Gain_dB;
                flag1 = FALSE;
            }

            aziIndex = 0;
            eleIndex++;
            image[patternIndex][eleIndex][aziIndex] = Gain_dB;
            prevElevationDegree = elevationDegree;
            prevAzimuthDegree = azimuthDegree;
            PrevGain_dB = Gain_dB;
            flag = TRUE;
        }

        if (azimuthDegree > prevAzimuthDegree)
        {
            delta = ((Gain_dB - PrevGain_dB)
                / ((azimuthDegree - prevAzimuthDegree) * ratio));

            for (i = 1; i < ((azimuthDegree - prevAzimuthDegree)
                * ratio); i++)
            {
                image[patternIndex][eleIndex][aziIndex + i]
                    = PrevGain_dB + delta * i;

                flag2 = TRUE;
            }

            if (flag2 == TRUE)
            {
                aziIndex = aziIndex + i - 1;
            }

            image[patternIndex][eleIndex][++aziIndex]
                    = Gain_dB;

            prevAzimuthDegree = azimuthDegree;
            PrevGain_dB = Gain_dB;
            flag2 = FALSE;
        }

        if ((eleIndex >= antennaPatterns->elevationResolution + 1)
            || (aziIndex >= antennaPatterns->azimuthResolution + 1)
            || (patternIndex >= antennaPatterns->numOfPatterns))
        {
            sprintf(err, " The pattern file is erronous\n");
            ERROR_ReportError(err);
        }

        if ((prevAzimuthDegree != azimuthDegree)
            || (prevElevationDegree != elevationDegree))
        {
            sprintf(err, " The pattern file is erronous\n");
            ERROR_ReportError(err);
        }
    }//end of for

    if (aziIndex != antennaPatterns->azimuthResolution)
    {
        std::string errorStr;
        errorStr = "Azimuth and Elavation resolution are not completely"
                    " filled out in ";
        errorStr += antennaPatterns->antennaPatternName;
        errorStr += "\nQualnet will fill the antenna pattern with its own"
                    " interpolated data\n";
        ERROR_ReportWarning(errorStr.c_str());
        while (aziIndex < antennaPatterns->azimuthResolution)
        {
            ++aziIndex;
            image[patternIndex][eleIndex][aziIndex]
                    = PrevGain_dB;
        }
    }
    if (eleIndex != (antennaPatterns->elevationResolution))
    {
        std::string errorStr;
        errorStr = "Azimuth and Elavation resolution are not completely"
                   " filled out in ";
        errorStr += antennaPatterns->antennaPatternName;
        errorStr += "\nQualnet will fill the antenna pattern with its own"
                    " interpolated data\n";
        ERROR_ReportWarning(errorStr.c_str());
        eleIndex++;
        while (eleIndex <= antennaPatterns->elevationResolution)
        {
            for (aziIndex = 0; aziIndex <= antennaPatterns->
                azimuthResolution ; aziIndex++)
            {
                image[patternIndex][eleIndex][aziIndex]
                    = image[patternIndex][eleIndex - 1][aziIndex];
            }
            eleIndex++;
        }
    }

    ERROR_Assert(patternIndex == antennaPatterns->numOfPatterns - 1 ,
        "Illegal number of patterns in the file.\n");

    antennaPatterns->antennaPatternElements = image;

}//end of the function.

// /**
// FUNCTION :: ANTENNA_Read2DAsciiPatterns
// LAYER :: Physical Layer.
// PURPOSE :: Used to read ASCII 2D pattern file.
// PARAMETERS ::
// + node : Node* : node being used.
// + phyIndex : int : interface for which physical
//                    to be initialized.
// +antennaInput : NodeInput* : structure containing contents of
//                              input file.
// +antennaPatterns : AntennaPattern* : Pointer to Global Antenna Pattern
//                                      structure.
// +azimuthPlane : BOOL : A boolean variable to differentiate the file
//                        azimuth or elevation.
// +conversionParameter : const float : conversion parameter to change
//                                      the dB values in dBi.
// RETURN :: void : NULL
// **/

void ANTENNA_Read2DAsciiPatterns(
     Node* node,
     int phyIndex,
     const NodeInput* antennaInput,
     AntennaPattern* antennaPatterns,
     BOOL azimuthPlane,
     const float conversionParameter)
{
    int i;
    int j;
    int k;
    char Token[MAX_STRING_LENGTH];
    char err[MAX_STRING_LENGTH];
    char* StrPtr;
    float PrevDegree = 0;
    float Degree;
    float Gain_dB;
    float PrevGain_dB = 1.0f;
    float ratio;
    int angleResolution;
    BOOL  flag = FALSE;
    BOOL  flag1 = FALSE;
    int numPatternIndex = -1;//numPatterns
    int  angleResolutionIndex = 0;//angleResolution
    AntennaPatternElement* antennaPatternElements;
    float delta;

    if (azimuthPlane == TRUE)
    {
        angleResolution = antennaPatterns->azimuthResolution;
        ratio = (float) angleResolution/360;
        antennaPatterns->antennaPatternElements
            = MEM_malloc(AZIMUTH_ELEVATION_INDEX
            * sizeof(AntennaPatternElement*));
    }
    else
    {
        angleResolution = antennaPatterns->elevationResolution;
        ratio = (float) angleResolution/180;
    }

    antennaPatternElements =
        (AntennaPatternElement*) MEM_malloc(
        antennaPatterns->numOfPatterns * sizeof(
        AntennaPatternElement));
    ERROR_Assert(antennaPatternElements ,
        "memory allocation problem in antennaPatternElements.\n");

    for (i = 0; i < antennaPatterns->numOfPatterns; i++)
    {
        antennaPatternElements[i].gains = (AntennaElementGain*)
            MEM_malloc((angleResolution + 1)
            * sizeof(AntennaElementGain));

        for (k = 0; k < angleResolution + 1; k++)
            antennaPatternElements[i].gains[k]
            = (-1) * ANTENNA_LOWEST_GAIN_dBi;
        antennaPatternElements[i].units
            = ANTENNA_GAIN_UNITS_DBI;
        antennaPatternElements[i].numGainValues
            = angleResolution;
    }


    for (j = 1; j < antennaInput->numLines; j++)
    {
        IO_GetToken(Token, antennaInput->inputStrings[j], &StrPtr);
        Degree = (float)atof(Token);

        IO_GetToken(Token, StrPtr, &StrPtr);
        Gain_dB = (float)atof(Token);
        Gain_dB *= -1;

        if (angleResolutionIndex == angleResolution)
        {
            flag = FALSE;
        }

        if ((angleResolutionIndex != angleResolution) && (Degree == 0.0)
            && (angleResolutionIndex != 0))
        {
            while (angleResolutionIndex < angleResolution)
            {

                antennaPatternElements[numPatternIndex].gains[
                    ++angleResolutionIndex] = PrevGain_dB;
            }
            flag = FALSE;
        }


        if (flag == FALSE)
        {
            ERROR_Assert(Degree == 0.0 , "Illegal azimuth degree.\n");
            angleResolutionIndex = 0;
            numPatternIndex++;
            ERROR_Assert(numPatternIndex < antennaPatterns->numOfPatterns,
                        "Illegal number of patterns in the file.\n");
            antennaPatternElements[numPatternIndex].gains[
                angleResolutionIndex] = Gain_dB;
            PrevDegree = Degree;
            PrevGain_dB = Gain_dB;
            flag = TRUE;
        }


        if (Degree > PrevDegree)
        {
            int i = 0;
            delta = ((Gain_dB - PrevGain_dB)/ ((Degree-PrevDegree)* ratio));
            for (i = 1; i < ((Degree-PrevDegree)* ratio); i++)
            {
                antennaPatternElements[numPatternIndex].gains[
                    angleResolutionIndex + i] = PrevGain_dB + delta * i;
                flag1 = TRUE;
            }
            if (flag1 == TRUE)
            {
                angleResolutionIndex = angleResolutionIndex + i - 1;
            }

            antennaPatternElements[numPatternIndex].gains
                [++angleResolutionIndex] = Gain_dB;
            PrevDegree = Degree;
            PrevGain_dB = Gain_dB;
            flag1 = FALSE;
        }

        if ((numPatternIndex >= antennaPatterns->numOfPatterns)
            || (angleResolutionIndex >= angleResolution + 1))
        {

            sprintf(err, " The pattern file is erronous\n");
            ERROR_ReportError(err);

        }

    }
    ERROR_Assert(numPatternIndex == antennaPatterns->numOfPatterns - 1 ,
        "Illegal number of antenna patterns in the file.\n");

    for (numPatternIndex = 0; numPatternIndex
        < antennaPatterns->numOfPatterns; numPatternIndex++)
    {

        for (angleResolutionIndex = 0; angleResolutionIndex
            <= angleResolution; angleResolutionIndex++)
        {
            antennaPatternElements[numPatternIndex].gains[
                angleResolutionIndex] =
                conversionParameter -
                antennaPatternElements[numPatternIndex].gains[
                angleResolutionIndex];
        }
    }

    AntennaPatternElement** elements = (AntennaPatternElement**)
        antennaPatterns->antennaPatternElements;
    if (azimuthPlane == TRUE)
    {
        elements[AZIMUTH_INDEX] = antennaPatternElements;
        elements[ELEVATION_INDEX] = NULL;
    }
    else
    {
        elements[ELEVATION_INDEX] = antennaPatternElements;
    }

}//end


// /**
// FUNCTION :: ANTENNA_IsInOmnidirectionalMode
// LAYER :: Physical Layer
// PURPOSE :: Is antenna in omnidirectional mode.
// PARAMETERS ::
// + node : Node* : node being used
// + phyIndex : int : interface for which physical to be use
// RETURN :: BOOL : returns TRUE if antenna is in
//                  omnidirectional mode
// **/

BOOL ANTENNA_IsInOmnidirectionalMode(Node* node, int phyIndex)
{
    PhyData* phyData = node->phyData[phyIndex];

    switch (phyData->antennaData->antennaModelType)
    {

        case ANTENNA_OMNIDIRECTIONAL:
        {
            return TRUE;
            break;
        }
        case ANTENNA_SWITCHED_BEAM:
        {
            return AntennaSwitchedBeamOmnidirectionalPattern(phyData);
            break;
        }
        case ANTENNA_STEERABLE:
        {
            return AntennaSteerableOmnidirectionalPattern(phyData);
            break;
        }
        case ANTENNA_PATTERNED:
        {
            return FALSE;
            break;
        }
        default:
        {
            char err[MAX_STRING_LENGTH];
            sprintf(err, "Unknown ANTENNA-MODEL for phy %d.\n", phyIndex);
            ERROR_ReportError(err);
            return 0;
            break;
        }
}

    // should never reach
    //  return TRUE;
}


// /**
// FUNCTION :: ANTENNA_ReturnHeight
// LAYER :: Physical Layer
// PURPOSE :: Return nodes antenna height.
// PARAMETERS ::
// + node : Node* : node being used
// + phyIndex : int : interface for which physical to be used
// RETURN :: float : height in meters
// **/

float ANTENNA_ReturnHeight(Node* node, int phyIndex)
{

    AntennaOmnidirectional* omniDirectional;
    AntennaSwitchedBeam* switchedBeam;
    AntennaSteerable* steerable;

    PhyData* phyData = node->phyData[phyIndex];

    switch (phyData->antennaData->antennaModelType)
    {
        case ANTENNA_OMNIDIRECTIONAL:
        {
            omniDirectional = (AntennaOmnidirectional*)
                phyData->antennaData->antennaVar;
            return omniDirectional->antennaHeight;
            break;
        }
        case ANTENNA_SWITCHED_BEAM:
        {
            switchedBeam = (AntennaSwitchedBeam*)
                phyData->antennaData->antennaVar;
            return switchedBeam->antennaHeight;
            break;
        }
        case ANTENNA_STEERABLE:
        {
            steerable = (AntennaSteerable*)
                phyData->antennaData->antennaVar;
            return steerable->antennaHeight;
            break;
        }
        case ANTENNA_PATTERNED:
        {
            AntennaPatterned *antennaPatterned;
            antennaPatterned = (AntennaPatterned*)
                node->phyData[phyIndex]->antennaData->antennaVar;
            return antennaPatterned->antennaHeight;
            break;
        }
        default:
        {
            return ANTENNA_DEFAULT_HEIGHT;
            break;
        }
    }

}


// /**
// FUNCTION :: ANTENNA_ReturnSystemLossIndB
// LAYER :: Physical Layer
// PURPOSE :: Return systen loss in dB.
// PARAMETERS ::
// + node : Node* : node being used
// + phyIndex : int : interface for which physical to be used
// RETURN :: double : loss in dB
// **/

double ANTENNA_ReturnSystemLossIndB(Node* node, int phyIndex)
{
    return node->phyData[phyIndex]->systemLoss_dB;
}

// /**
// FUNCTION :: ANTENNA_ReturnPatternIndex
// LAYER :: Physical Layer
// PURPOSE :: Return nodes current pattern index.
// PARAMETERS ::
// + node : Node* : node being used
// + phyIndex : int : interface for which physical to use
// RETURN :: int : returns pattern index
// **/

int ANTENNA_ReturnPatternIndex(Node* node, int phyIndex)
{
    PhyData *phyData = node->phyData[phyIndex];

    switch (phyData->antennaData->antennaModelType)
    {

        case ANTENNA_OMNIDIRECTIONAL:
        {
            return ANTENNA_OMNIDIRECTIONAL_PATTERN;
            break;
        }

        case ANTENNA_SWITCHED_BEAM:
        {
            return AntennaSwitchedBeamReturnPatternIndex(node, phyIndex);
            break;
        }

        case ANTENNA_STEERABLE:
        {
            return AntennaSteerableReturnPatternIndex(node, phyIndex);
            break;
        }

        case ANTENNA_PATTERNED:
        {
            return AntennaPatternedReturnPatternIndex(node, phyIndex);
        }

        default:
        {
            char err[MAX_STRING_LENGTH];
            sprintf(err, "Unknown ANTENNA-MODEL for phy %d.\n", phyIndex);
            ERROR_ReportError(err);
            return 0;
        }
    }
}

// /**
// FUNCTION :: ANTENNA_GainForThisDirection
// LAYER :: Physical Layer
// PURPOSE :: Return gain for this direction in dB.
// PARAMETERS ::
// + node : Node* : node being used
// + phyIndex : int : interface for which physical to be used
// + DOA : Orientation : direction of antenna
// RETURN :: float : gain in dB
// **/

float ANTENNA_GainForThisDirection(Node* node,
                                   int phyIndex,
                                   Orientation DOA)
{
    PhyData* phyData = node->phyData[phyIndex];
    float antennaMaxGain_dBi = ANTENNA_LOWEST_GAIN_dBi;

    switch (phyData->antennaData->antennaModelType)
    {

        case ANTENNA_OMNIDIRECTIONAL:
        {
            AntennaOmnidirectional* omniDirectional;
            omniDirectional
              = (AntennaOmnidirectional*)phyData->antennaData->antennaVar;
            antennaMaxGain_dBi = omniDirectional->antennaGain_dB;
            break;
        }
        case ANTENNA_SWITCHED_BEAM:
        {
            antennaMaxGain_dBi
                = AntennaSwitchedBeamGainForThisDirection(node,
                phyIndex, DOA);
            break;
        }
        case ANTENNA_STEERABLE:
        {
            antennaMaxGain_dBi
              = AntennaSteerableGainForThisDirection(node, phyIndex, DOA);
            break;
        }
        case ANTENNA_PATTERNED:
        {
            antennaMaxGain_dBi
              = AntennaPatternedGainForThisDirection(node, phyIndex, DOA);
            break;
        }
        default:
        {
            char err[MAX_STRING_LENGTH];
            sprintf(err, "Unknown ANTENNA-MODEL for phy %d.\n", phyIndex);
            ERROR_ReportError(err);
            break;
        }
    }

    ERROR_Assert(antennaMaxGain_dBi != ANTENNA_LOWEST_GAIN_dBi ,
        "antennaMaxGain_dBi is equal to the ANTENNA_LOWEST_GAIN_dBi.\n");

    return antennaMaxGain_dBi;
}


// /**
// FUNCTION :: ANTENNA_GainForThisDirectionWithPatternIndex
// LAYER :: Physical Layer
// PURPOSE ::  Return gain for this direction for the specified pattern in
//             dB.
// PARAMETERS ::
// + node : Node* : node being used
// + phyIndex : int : interface for which physical to be used
// + patternIndex : int : pattern index to use
// + DOA : Orientation : direction of antenna
// RETURN :: float : gain in dB
// **/

float ANTENNA_GainForThisDirectionWithPatternIndex(Node* node,
                                                   int phyIndex,
                                                   int patternIndex,
                                                   Orientation DOA)
{
    PhyData* phyData = node->phyData[phyIndex];
    float antennaMaxGain_dBi = ANTENNA_LOWEST_GAIN_dBi;

    switch (phyData->antennaData->antennaModelType)
    {

        case ANTENNA_OMNIDIRECTIONAL:
        {
            AntennaOmnidirectional* omniDirectional;
            omniDirectional
               = (AntennaOmnidirectional*)phyData->antennaData->antennaVar;
            antennaMaxGain_dBi = omniDirectional->antennaGain_dB;
            break;
        }
        case ANTENNA_SWITCHED_BEAM:
        {
            antennaMaxGain_dBi
                = AntennaSwitchedBeamGainForThisDirectionWithPatternIndex(
                node, phyIndex, patternIndex, DOA);
            break;
        }
        case ANTENNA_STEERABLE:
        {
            antennaMaxGain_dBi
                = AntennaSteerableGainForThisDirectionWithPatternIndex(
                node, phyIndex, patternIndex, DOA);
            break;
        }
        case ANTENNA_PATTERNED:
        {
            antennaMaxGain_dBi
                = AntennaPatternedGainForThisDirectionWithPatternIndex(
                node, phyIndex, patternIndex, DOA);
            break;
        }
        default:
        {
            char err[MAX_STRING_LENGTH];
            sprintf(err, "Unknown ANTENNA-MODEL for phy %d.\n", phyIndex);
            ERROR_ReportError(err);
            break;
        }
    }

    ERROR_Assert(antennaMaxGain_dBi != ANTENNA_LOWEST_GAIN_dBi ,
        "antennaMaxGain_dBi is equal to the ANTENNA_LOWEST_GAIN_dBi.\n");

    return antennaMaxGain_dBi;
}

// /**
// FUNCTION :: ANTENNA_GainForThisSignal
// LAYER :: Physical Layer
// PURPOSE :: Return gain in dB.
// PARAMETERS ::
// + node : Node* : node being used
// + phyIndex : int : interface for which physical to be used
// + propRxInfo : PropRxInfo* : receiver propagation info
// RETURN :: float : gain in dB
// **/

float ANTENNA_GainForThisSignal(Node* node,
                                int phyIndex,
                                PropRxInfo* propRxInfo)
{
#ifdef AGI_INTERFACE
    if (node->partitionData->isAgiInterfaceEnabled)
    {
        Node* txNode;
        BOOL nodeIsLocal =
            PARTITION_ReturnNodePointer(node->partitionData,
                                        &txNode,
                                        propRxInfo->txNodeId,
                                        TRUE);
        assert(nodeIsLocal);
        assert(txNode != NULL);

        PhyData* txPhy = txNode->phyData[propRxInfo->txPhyIndex];
        PhyData* rxPhy = node->phyData[phyIndex];

        NodeInterfacePair xmtrId(txNode->nodeId, txPhy->macInterfaceIndex);
        NodeInterfacePair rcvrId(node->nodeId, rxPhy->macInterfaceIndex);

        CAgiInterfaceUtil::ComputeRequest req;
        req.time_nanoseconds = propRxInfo->rxStartTime;
        req.xmtrId = xmtrId;
        req.rcvrId = rcvrId;
        req.channelIndex = propRxInfo->channelIndex;
        req.frequency_hertz = propRxInfo->frequency;

        double gain = 0.0;
        CAgiInterfaceUtil::GetInstance().ComputeReceiveGain(req, gain);
        return static_cast<float>(gain);
    }
    else
#endif
    {
        return ANTENNA_GainForThisDirection(node,
            phyIndex, propRxInfo->rxDOA);
    }
}


// /**
// FUNCTION :: ANTENNA_DefaultGainForThisSignal
// LAYER :: Physical Layer
// PURPOSE :: Return default gain in dB.
// PARAMETERS ::
// + node : Node* : node being used
// + phyIndex : int : interface for which physical to be used
// + propRxInfo : PropRxInfo* : receiver propagation info
// RETURN :: float : gain in dB
// **/

float ANTENNA_DefaultGainForThisSignal(Node* node,
                                       int phyIndex,
                                       PropRxInfo* propRxInfo)
{
#ifdef AGI_INTERFACE
    if (node->partitionData->isAgiInterfaceEnabled)
    {
        Node* txNode;
        BOOL nodeIsLocal =
            PARTITION_ReturnNodePointer(node->partitionData,
                                        &txNode,
                                        propRxInfo->txNodeId,
                                        TRUE);
        assert(nodeIsLocal);
        assert(txNode != NULL);

        PhyData* txPhy = txNode->phyData[propRxInfo->txPhyIndex];
        PhyData* rxPhy = node->phyData[phyIndex];

        NodeInterfacePair xmtrId(txNode->nodeId, txPhy->macInterfaceIndex);
        NodeInterfacePair rcvrId(node->nodeId, rxPhy->macInterfaceIndex);

        CAgiInterfaceUtil::ComputeRequest req;
        req.time_nanoseconds = propRxInfo->rxStartTime;
        req.xmtrId = xmtrId;
        req.rcvrId = rcvrId;
        req.channelIndex = propRxInfo->channelIndex;
        req.frequency_hertz = propRxInfo->frequency;

        double gain = 0.0;
        CAgiInterfaceUtil::GetInstance().ComputeReceiveGain(req, gain);
        return static_cast<float>(gain);
    }
    else
#endif
    {
        PhyData* phyData = node->phyData[phyIndex];
        AntennaOmnidirectional* omniDirectional;
        AntennaSwitchedBeam* switchedBeam;
        AntennaSteerable* steerable;

        switch (phyData->antennaData->antennaModelType)
        {
            case ANTENNA_OMNIDIRECTIONAL:
            {
                omniDirectional =
                    (AntennaOmnidirectional*)phyData->antennaData->antennaVar;
                return omniDirectional->antennaGain_dB;
                break;
            }
            case ANTENNA_SWITCHED_BEAM:
            {
                switchedBeam =
                    (AntennaSwitchedBeam*)phyData->antennaData->antennaVar;
                return switchedBeam->antennaGain_dB;
                break;
            }
            case ANTENNA_STEERABLE:
            {
                steerable =
                    (AntennaSteerable*)phyData->antennaData->antennaVar;
                return steerable->antennaGain_dB;
                break;
            }
            case ANTENNA_PATTERNED:
            {
                return AntennaPatternedGainForThisDirection(node, phyIndex,
                    propRxInfo->rxDOA);
                break;
            }
            default:
            {
                return ANTENNA_DEFAULT_GAIN_dBi;
                break;
            }
        }
    }
}


// /**
// FUNCTION :: ANTENNA_LockAntennaDirection
// LAYER :: Physical Layer
// PURPOSE :: Lock antenna to current direction.
// PARAMETERS ::
// + node : Node* : node being used
// + phyIndex : int : interface for which physical to be used
// RETURN :: void : NULL
// **/

void ANTENNA_LockAntennaDirection(Node* node, int phyIndex)
{
#ifdef AGI_INTERFACE
    if (!node->partitionData->isAgiInterfaceEnabled)
#endif
    {
        PhyData* phyData = node->phyData[phyIndex];

        switch (phyData->antennaData->antennaModelType)
        {

            case ANTENNA_OMNIDIRECTIONAL:
            {
                break;
            }

            case ANTENNA_SWITCHED_BEAM:
            {
                AntennaSwitchedBeamLockAntennaDirection(node, phyIndex);
                break;
            }
            case ANTENNA_STEERABLE:
            {
                AntennaSteerableLockAntennaDirection(node, phyIndex);
                break;
            }

            case ANTENNA_PATTERNED:
            {
                AntennaPatternedLockAntennaDirection(node, phyIndex);
                break;
            }

            default:
            {
                char err[MAX_STRING_LENGTH];
                sprintf(err, "Unknown ANTENNA-MODEL for phy %d.\n", phyIndex);
                ERROR_ReportError(err);
                break;
            }

        }//switch//
    }
}

// /**
// FUNCTION :: ANTENNA_UnlockAntennaDirection
// LAYER :: Physical Layer
// PURPOSE :: Unlock antenna.
// PARAMETERS ::
// + node : Node* : node being used
// + phyIndex : int : interface for which physical to be used
// RETURN :: void : NULL
// **/

void ANTENNA_UnlockAntennaDirection(Node* node, int phyIndex)
{
#ifdef AGI_INTERFACE
    if (!node->partitionData->isAgiInterfaceEnabled)
#endif
    {
        PhyData* phyData = node->phyData[phyIndex];

        switch (phyData->antennaData->antennaModelType)
        {

            case ANTENNA_OMNIDIRECTIONAL:
            {
                break;
            }
            case ANTENNA_SWITCHED_BEAM:
            {
                AntennaSwitchedBeamUnlockAntennaDirection(node, phyIndex);
                break;
            }
            case ANTENNA_STEERABLE:
            {
                AntennaSteerableUnlockAntennaDirection(node, phyIndex);
                break;
            }
            case ANTENNA_PATTERNED:
            {
                AntennaPatternedUnlockAntennaDirection(node, phyIndex);
                break;
            }
            default:
            {
                char err[MAX_STRING_LENGTH];
                sprintf(err, "Unknown ANTENNA-MODEL for phy %d.\n", phyIndex);
                ERROR_ReportError(err);
                break;
            }
        }//switch//
    }
}


// /**
// FUNCTION :: ANTENNA_IsLocked
// LAYER :: Physical Layer
// PURPOSE :: Return if antenna is locked.
// PARAMETERS ::
// + node : Node* : node being used
// + phyIndex : int : interface for which physical to be used
// RETURN :: BOOL : Returns TRUE if antenna is locked.
// **/

BOOL ANTENNA_IsLocked(Node* node, int phyIndex)
{
#ifdef AGI_INTERFACE
    if (node->partitionData->isAgiInterfaceEnabled)
    {
        return TRUE;
    }
    else
#endif
    {
        PhyData* phyData = node->phyData[phyIndex];

        switch (phyData->antennaData->antennaModelType)
        {

            case ANTENNA_OMNIDIRECTIONAL:
            {
                return TRUE;
                break;
            }

            case ANTENNA_SWITCHED_BEAM:
            {
                return AntennaSwitchedBeamIsLocked(node, phyIndex);
                break;
            }

            case ANTENNA_STEERABLE:
            {
                return AntennaSteerableIsLocked(node, phyIndex);
                break;
            }
            case ANTENNA_PATTERNED:
            {
                return AntennaPatternedIsLocked(node, phyIndex);
                break;
            }
            default:
            {
                char err[MAX_STRING_LENGTH];
                sprintf(err, "Unknown ANTENNA-MODEL for phy %d.\n", phyIndex);
                ERROR_ReportError(err);
                return 0;
                break;
            }
        }//switch//
    }
}


// /**
// FUNCTION :: ANTENNA_SetToDefaultMode
// LAYER :: Physical Layer
// PURPOSE :: Set default antenna mode (usally omni).
// PARAMETERS ::
// + node : Node* : node being used
// + phyIndex : int : interface for which physical to be used
// RETURN :: void : NULL
// **/

void ANTENNA_SetToDefaultMode(Node* node, int phyIndex)
{
#ifdef AGI_INTERFACE
    if (!node->partitionData->isAgiInterfaceEnabled)
#endif
    {
        PhyData* phyData = node->phyData[phyIndex];
        Int32 patternIndex = ANTENNA_OMNIDIRECTIONAL_PATTERN;
        switch (phyData->antennaData->antennaModelType)
        {

            case ANTENNA_SWITCHED_BEAM:
            {
                AntennaSwitchedBeamSetPattern(node, phyIndex,
                    ANTENNA_OMNIDIRECTIONAL_PATTERN);
                break;
            }

            case ANTENNA_STEERABLE:
            {
                AntennaSteerableSetPattern(node, phyIndex,
                    ANTENNA_OMNIDIRECTIONAL_PATTERN);
                break;
            }

            case ANTENNA_PATTERNED:
            {
                AntennaPatternedSetPattern(node, phyIndex,
                    ANTENNA_DEFAULT_PATTERN);
                patternIndex = ANTENNA_DEFAULT_PATTERN;
                break;
            }

            default:
            {
                char err[MAX_STRING_LENGTH];
                sprintf(err, "Unknown ANTENNA-MODEL for phy %d.\n", phyIndex);
                ERROR_ReportError(err);
                break;
            }
        }//switch//

        //GuiStart
        if (node->guiOption)
        {
            GUI_SetPatternIndex(
                node,
                phyData->macInterfaceIndex,
                patternIndex,
                node->getNodeTime());
        }
        //GuiEnd
    }
}


// /**
// FUNCTION :: ANTENNA_SetToBestGainConfigurationForThisSignal
// PURPOSE :: Set antenna for best gain using the Rx info.
// PARAMETERS ::
// + node : Node* : node being used
// + phyIndex : int : interface for which physical to be used
// + propRxInfo : PropRxInfo* : receiver propagation info
// RETURN :: void : NULL
// **/

void ANTENNA_SetToBestGainConfigurationForThisSignal(Node* node,
                                                     int phyIndex,
                                           PropRxInfo* propRxInfo)
{
#ifdef AGI_INTERFACE
    if (!node->partitionData->isAgiInterfaceEnabled)
#endif
    {
        PhyData* phyData = node->phyData[phyIndex];

        switch (phyData->antennaData->antennaModelType)
        {

            case ANTENNA_SWITCHED_BEAM:
            {
                int patternIndex;
                float NotUsed;

                AntennaSwitchedBeamMaxGainPatternForThisSignal(
                    node, phyIndex, propRxInfo,
                    &patternIndex, &NotUsed);

                AntennaSwitchedBeamSetPattern(node, phyIndex, patternIndex);

                if (node->guiOption){
                    GUI_SetPatternIndex(
                        node,
                        phyData->macInterfaceIndex,
                        patternIndex,
                        node->getNodeTime());
                }

                break;
            }

            case ANTENNA_STEERABLE:
            {
                int patternIndex;
                float NotUsed;

                AntennaSteerableMaxGainPatternForThisSignal(
                    node, phyIndex, propRxInfo,
                    &patternIndex, &NotUsed);

                if (patternIndex != ANTENNA_OMNIDIRECTIONAL_PATTERN)
                {
                    AntennaSteerableSetMaxGainSteeringAngle(
                        node,
                        phyIndex,
                        propRxInfo->rxDOA);
                }
                else
                {
                    AntennaSteerableSetPattern(node,
                                               phyIndex,
                                               patternIndex);
                }

                if (node->guiOption)
                {
                    AntennaSteerableGetPattern(node,
                        phyIndex, &patternIndex);
                    GUI_SetPatternAndAngle(node,
                        phyData->macInterfaceIndex,
                        patternIndex,
                        propRxInfo->rxDOA.azimuth,
                        propRxInfo->rxDOA.elevation,
                        node->getNodeTime());
                }

                break;
            }

            case ANTENNA_PATTERNED:
            {

                int patternIndex;
                float NotUsed;

                AntennaPatternedMaxGainPatternForThisSignal(
                    node, phyIndex, propRxInfo,
                    &patternIndex, &NotUsed);

                AntennaPatternedSetPattern(node, phyIndex, patternIndex);

                if (node->guiOption) {
                    GUI_SetPatternIndex(
                        node,
                        phyData->macInterfaceIndex,
                        patternIndex,
                        node->getNodeTime());
                }

                break;
            }

            default:
            {
                char err[MAX_STRING_LENGTH];
                sprintf(err, "Unknown ANTENNA-MODEL for phy %d.\n", phyIndex);
                ERROR_ReportError(err);
                break;
            }
        }//switch//
    }
}


// /**
// FUNCTION :: ANTENNA_SetBestConfigurationForAzimuth
// LAYER :: Physical Layer
// PURPOSE :: Set antenna for best gain using the azimuth.
// PARAMETERS ::
// + node : Node* : node being used
// + phyIndex : int : interface for which physical to be used
// + azimuth : double : the azimuth
// RETURN :: void : NULL
// **/

void ANTENNA_SetBestConfigurationForAzimuth(Node* node,
                                            int phyIndex,
                                            double azimuth)
{
#ifdef AGI_INTERFACE
    if (!node->partitionData->isAgiInterfaceEnabled)
#endif
    {
        PhyData* phyData = node->phyData[phyIndex];

        switch (phyData->antennaData->antennaModelType)
        {

            case ANTENNA_SWITCHED_BEAM:
            {

                AntennaSwitchedBeamSetBestPatternForAzimuth(node,
                    phyIndex, azimuth);

                if (node->guiOption)
                {
                    int patternIndex;
                    AntennaSwitchedBeamGetPattern(node,
                        phyIndex, &patternIndex);

                    GUI_SetPatternIndex(node,
                        phyData->macInterfaceIndex,
                        patternIndex,
                        node->getNodeTime());
                }
                break;
            }

            case ANTENNA_STEERABLE:
            {

                AntennaSteerableSetBestConfigurationForAzimuth(
                    node, phyIndex, azimuth);

                if (node->guiOption)
                {
                    int patternIndex;
                    AntennaSteerableGetPattern(node,
                        phyIndex, &patternIndex);
                    GUI_SetPatternAndAngle(node,
                        phyData->macInterfaceIndex,
                        patternIndex,
                        (int) azimuth,
                        0,
                        node->getNodeTime());
                }
                break;
            }

            case ANTENNA_PATTERNED:
            {

                AntennaPatternedSetBestPatternForAzimuth(
                    node, phyIndex, azimuth);

                //GuiStart
                if (node->guiOption)
                {
                    int patternIndex;
                    AntennaPatternedGetPattern(node,
                        phyIndex, &patternIndex);

                    GUI_SetPatternIndex(node,
                        phyData->macInterfaceIndex,
                        patternIndex,
                        node->getNodeTime());
                }
                //GuiEnd
                break;
            }

            default:
            {
                char err[MAX_STRING_LENGTH];
                sprintf(err, "Unknown ANTENNA-MODEL for phy %d.\n", phyIndex);
                ERROR_ReportError(err);
                break;
            }
        }//switch//
    }
}

// /**
// FUNCTION :: ANTENNA_GetSteeringAngle
// LAYER :: Physical Layer
// PURPOSE :: Get steering angle of the antenna.
// PARAMETERS ::
// + node : Node* : node being used
// + phyIndex : int : interface for which physical to be used
// + angle    : Orientation* : For returning the angle
// RETURN :: void : NULL
// **/

void ANTENNA_GetSteeringAngle(
     Node* node,
     int phyIndex,
     Orientation* angle)
{
    angle->azimuth = 0;
    angle->elevation = 0;

#ifdef AGI_INTERFACE
    if (!node->partitionData->isAgiInterfaceEnabled)
#endif
    {
        PhyData* phyData = node->phyData[phyIndex];

        switch (phyData->antennaData->antennaModelType)
        {
            case ANTENNA_STEERABLE:
            {

                AntennaSteerableGetSteeringAngle(node, phyIndex, angle);

                break;
            }
            default:
                break;
        }
    }
}

// /**
// FUNCTION :: ANTENNA_SetSteeringAngle
// LAYER :: Physical Layer
// PURPOSE :: Set the steering angle of the antenna
// PARAMETERS ::
// + node : Node* : node being used
// + phyIndex : int : interface for which physical to be used
// + angle : Orientation : Steering angle to be
// RETURN :: void : NULL
// **/

void ANTENNA_SetSteeringAngle(
     Node* node,
     int phyIndex,
     Orientation angle)
{
#ifdef AGI_INTERFACE
    if (!node->partitionData->isAgiInterfaceEnabled)
#endif
    {
        PhyData* phyData = node->phyData[phyIndex];

        switch (phyData->antennaData->antennaModelType)
        {
            case ANTENNA_STEERABLE:
            {

                AntennaSteerableSetMaxGainSteeringAngle(
                    node,
                    phyIndex,
                    angle);

                break;
            }
            default:
                break;
        }
    }
}

// /**
// FUNCTION :: ANTENNA_ReturnNsmaPatternFile
// LAYER :: Physical Layer
// PURPOSE :: Read in the NSMA pattern .
// PARAMETERS ::
// + node : Node* : node being used
// + phyIndex : int : interface for which
//                    physical to be initialized
// + antennaModelInput : const NodeInput* : structure containing
//                                          contents of input
//                                          file
// +antennaPatterns : AntennaPatterns* : Pointer to
//                                       the global antenna
//                                       pattern structure.
// RETURN :: void : NULL
// /**

void ANTENNA_ReturnNsmaPatternFile(
     Node* node,
     int  phyIndex,
     const NodeInput* antennaModelInput,
     AntennaPattern* antennaPatterns)
{
    PhyData* phyData = node->phyData[phyIndex];

    BOOL wasFound;
    int numPatterns;
    float conversionParameter;
    int patternSetRepeatAngle;

    int i;
    int j;
    int azimuthResolution;
    int elevationResolution;
    float aziRatio = 1.0;
    float elvRatio = 1.0;

    double highestGain;
    int    highestGainAngle;
    NodeInput antennaInput;

    float** patternAzimuth_dBi;
    float** patternElevation_dB;
    AntennaPatternElement** antennaPatternElements;
    char err[MAX_STRING_LENGTH];
    NSMAPatternVersion version;

    IO_ReadFloat(
        node,
        node->nodeId,
        phyData->macInterfaceIndex,
        antennaModelInput,
        "ANTENNA-PATTERN-CONVERSION-PARAMETER",
        &wasFound,
        &conversionParameter);

    if (!wasFound)
    {
        sprintf(err, "Error: ANTENNA-PATTERN-CONVERSION-PARAMETER "
            "is missing for the antenna with nodeId = %d\n",
            node->nodeId);
        ERROR_ReportError(err);

    }

    IO_ReadCachedFile(
        node,
        node->nodeId,
        phyData->macInterfaceIndex,
        antennaModelInput,
        "ANTENNA-PATTERN-PATTERN-FILE",
        &wasFound,
        &antennaInput);

    if (!wasFound)
    {
        sprintf(err, "Error: ANTENNA-PATTERN-PATTERN-FILE "
            "is missing for the antenna with nodeId = %d\n",
            node->nodeId);
        ERROR_ReportError(err);
    }

    strcpy(antennaPatterns->antennaFileName, antennaInput.ourName);

    IO_ReadInt(
        node,
        node->nodeId,
        phyData->macInterfaceIndex,
        antennaModelInput,
        "ANTENNA-PATTERN-STEERABLE-SET-REPEAT-ANGLE",
        &wasFound,
        &patternSetRepeatAngle);

    if (!wasFound)
    {
        antennaPatterns->patternSetRepeatAngle = 360;
    }
    else
    {
        antennaPatterns->patternSetRepeatAngle
            = patternSetRepeatAngle;
    }

    IO_ReadInt(
        node,
        node->nodeId,
        phyData->macInterfaceIndex,
        antennaModelInput,
        "ANTENNA-PATTERN-NUM-PATTERNS",
        &wasFound,
        &numPatterns);

    if (!wasFound)
    {
        ERROR_ReportError(
            "ANTENNA-PATTERN-NUM-PATTERNS is missing for the antenna.\n");
        //exit from here
    }

    ERROR_Assert(numPatterns > 0 ,
        "Illegal number of patterns in the file.\n");

    if (numPatterns > 1)
    {
        ERROR_ReportWarning(
            "multiple set of patterns are specified in a file \n");
        // Only first pattern will be considered
        numPatterns = 1;
    }

    antennaPatterns->numOfPatterns = numPatterns;

    IO_ReadInt(
        node,
        node->nodeId,
        phyData->macInterfaceIndex,
        antennaModelInput,
        "ANTENNA-PATTERN-AZIMUTH-RESOLUTION",
        &wasFound,
        &azimuthResolution);

    if (!wasFound)
    {
        azimuthResolution = 360;
    }
    IO_ReadInt(
        node,
        node->nodeId,
        phyData->macInterfaceIndex,
        antennaModelInput,
        "ANTENNA-PATTERN-ELEVATION-RESOLUTION",
        &wasFound,
        &elevationResolution);

    if (!wasFound)
    {
        elevationResolution = -1;

    }
    ANTENNA_ReadNsmaPatterns(
        node,
        phyIndex,
        &antennaInput,
        numPatterns,
        &patternAzimuth_dBi,
        &azimuthResolution,
        &patternElevation_dB,
        &elevationResolution,
        &version);

    antennaPatterns->is3DArray = FALSE;
    antennaPatterns->azimuthUnits
        = ANTENNA_PATTERN_UNITS_DEGREES_MINUS_180_TO_180;
    antennaPatterns->maxAzimuthIndex = 180;
    antennaPatterns->azimuthResolution = azimuthResolution;
    antennaPatterns->elevationResolution = elevationResolution;
    aziRatio = (float)(antennaPatterns->azimuthResolution/360.0);

    if (version == NSMA_TRADITIONAL)
    {
        antennaPatterns->is3DGeometry = TRUE;
        antennaPatterns->elevationUnits =
            ANTENNA_PATTERN_UNITS_DEGREES_MINUS_90_TO_90;
        antennaPatterns->maxElevationIndex = 90;
        elvRatio = (float)(antennaPatterns->elevationResolution/180.0);
    }
    else
    {
        antennaPatterns->is3DGeometry = FALSE;
        antennaPatterns->elevationUnits =
            ANTENNA_PATTERN_UNITS_DEGREES_MINUS_180_TO_180;
        antennaPatterns->maxElevationIndex = 180;
        elvRatio = (float)(antennaPatterns->elevationResolution/360.0);
    }

    antennaPatternElements = (AntennaPatternElement**)
        MEM_malloc(AZIMUTH_ELEVATION_INDEX
        * sizeof(AntennaPatternElement*));
    antennaPatternElements[AZIMUTH_INDEX] = (AntennaPatternElement*)
        MEM_malloc(numPatterns * sizeof(AntennaPatternElement));
    antennaPatternElements[ELEVATION_INDEX] = (AntennaPatternElement*)
        MEM_malloc(numPatterns * sizeof(AntennaPatternElement));


    antennaPatterns->boreSightAngle
        = (Orientation *)MEM_malloc(numPatterns * sizeof(Orientation));
    antennaPatterns->boreSightGain_dBi
        = (float*)MEM_malloc(numPatterns * sizeof(float));


    for (i = 0; i < numPatterns; i++)
    {
        highestGain = ANTENNA_LOWEST_GAIN_dBi;
        highestGainAngle = 0;


        antennaPatternElements[AZIMUTH_INDEX][i].units
            = ANTENNA_GAIN_UNITS_DBI;
        antennaPatternElements[AZIMUTH_INDEX][i].numGainValues
            = azimuthResolution;
        antennaPatternElements[AZIMUTH_INDEX][i].gains
            = (AntennaElementGain*) MEM_malloc(
            (azimuthResolution + 1)
                * sizeof(AntennaElementGain));


        for (j = 0; j <= azimuthResolution; j++)
        {
            patternAzimuth_dBi[i][j] = conversionParameter
                - patternAzimuth_dBi[i][j];
            antennaPatternElements[AZIMUTH_INDEX][i].gains[j]
                = patternAzimuth_dBi[i][j];

            if (DEBUG_NSMA)
            {
                printf("patternIndex %d\tangle %f\tgain %f\n", i, j/aziRatio,
                    antennaPatternElements[AZIMUTH_INDEX][i].gains[j]);
            }

            if (patternAzimuth_dBi[i][j] > highestGain)
            {
                highestGain = patternAzimuth_dBi[i][j];
                highestGainAngle = j;
            }
        }
        ERROR_Assert(highestGain != ANTENNA_LOWEST_GAIN_dBi ,
            "highest gain is equal to ANTENNA_LOWEST_GAIN_dBi.\n");

        antennaPatterns->boreSightAngle[i].azimuth = (short)
            ((int)(highestGainAngle / aziRatio));
        antennaPatterns->boreSightGain_dBi[i] = (float)highestGain;
        MEM_free(patternAzimuth_dBi[i]);
    }
    MEM_free(patternAzimuth_dBi);

    if (!patternElevation_dB)
    {
        for (i = 0; i < numPatterns; i++)
        {
            antennaPatterns->boreSightAngle[i].elevation = 0;
        }

        MEM_free(antennaPatternElements[ELEVATION_INDEX]);
        antennaPatternElements[ELEVATION_INDEX] = NULL;
        antennaPatterns->antennaPatternElements =
            antennaPatternElements;
        return;
    }


    for (i = 0; i < numPatterns; i++)
    {
        highestGain = ANTENNA_LOWEST_GAIN_dBi;
        highestGainAngle = 0;

        antennaPatternElements[ELEVATION_INDEX][i].units
            = ANTENNA_GAIN_UNITS_DBI;
        antennaPatternElements[ELEVATION_INDEX][i].numGainValues
            = (int)(ANGLE_RESOLUTION * elvRatio);
        antennaPatternElements[ELEVATION_INDEX][i].gains
            = (AntennaElementGain*)
            MEM_malloc((elevationResolution + 1) * sizeof(
            AntennaElementGain));

        for (j = 0; j <= elevationResolution; j++)
        {
            patternElevation_dB[i][j] = conversionParameter
                - patternElevation_dB[i][j];
            antennaPatternElements[ELEVATION_INDEX][i].gains[j]
                = patternElevation_dB[i][j];

            if (DEBUG_NSMA)
            {
                printf("patternIndex %d\tangle %f\tgain %f\n", i, j/elvRatio,
                    antennaPatternElements[ELEVATION_INDEX][i].gains[j]);
            }

            if (patternElevation_dB[i][j] > highestGain)
            {
                highestGain = patternElevation_dB[i][j];
                highestGainAngle = j;
            }
        }
        ERROR_Assert(highestGain != ANTENNA_LOWEST_GAIN_dBi ,
            "highest gain is equal to ANTENNA_LOWEST_GAIN_dBi.\n");

        if (version == NSMA_TRADITIONAL)
        {
        antennaPatterns->boreSightAngle[i].elevation = (short)
            ((int)(highestGainAngle / elvRatio) - (ANGLE_RESOLUTION / 4));
        }
        else
        {
            antennaPatterns->boreSightAngle[i].elevation = (short)
                ((int)(highestGainAngle / elvRatio) - (ANGLE_RESOLUTION / 2));
        }
        antennaPatterns->boreSightGain_dBi[i] += (float)
            highestGain;
    }

    antennaPatterns->antennaPatternElements = antennaPatternElements;

}//end of function


// /**
// FUNCTION :: ANTENNA_ReturnAsciiPatternFile
// LAYER :: Physical Layer
// PURPOSE :: Read in the ASCII pattern .
// PARAMETERS ::
// + node : Node* : node being used
// + phyIndex : int : interface for which physical
//                    to be initialized
// + antennaModelInput : const NodeInput* : structure containing
//                                          contents of input
//                                          file
// +antennaPatterns : AntennaPatterns* : Pointer to the global
//                                       antenna pattern structure.
// RETURN :: void : NULL
// **/

void ANTENNA_ReturnAsciiPatternFile(
     Node* node,
     int phyIndex,
     const NodeInput*  antennaModelInput,
     AntennaPattern* antennaPatterns)
{
    BOOL wasFound;
    BOOL wasFound1 = FALSE;
    int numPatterns;
    int i;
    int j;
    int k;
    float highestele = 0.0;
    float highestazi = 0.0;
    int patternSetRepeatAngle;
    float   highestGain;
    float highestGainAngle;
    NodeInput antennaInput;
    NodeInput antennaInput1;
    float conversionParameter;

    PhyData* phyData = node->phyData[phyIndex];
    char err[MAX_STRING_LENGTH];

    antennaPatterns->azimuthUnits = ANTENNA_PATTERN_UNITS_DEGREES_0_TO_360;
    antennaPatterns->elevationUnits
        = ANTENNA_PATTERN_UNITS_DEGREES_0_TO_180;


    IO_ReadFloat(
        node,
        node->nodeId,
        phyData->macInterfaceIndex,
        antennaModelInput,
        "ANTENNA-PATTERN-CONVERSION-PARAMETER",
        &wasFound,
        &conversionParameter);

    if (!wasFound)
    {
        sprintf(err, "Error: ANTENNA-PATTERN-CONVERSION-PARAMETER "
            "is missing for the antenna with nodeId = %d\n",
            node->nodeId);
        ERROR_ReportError(err);

    }

    IO_ReadInt(
        node,
        node->nodeId,
        phyData->macInterfaceIndex,
        antennaModelInput,
        "ANTENNA-PATTERN-STEERABLE-SET-REPEAT-ANGLE",
        &wasFound,
        &patternSetRepeatAngle);

    if (!wasFound)
    {
        antennaPatterns->patternSetRepeatAngle = 360;
    }
    else
    {
        antennaPatterns->patternSetRepeatAngle = patternSetRepeatAngle;
    }

    IO_ReadInt(
        node,
        node->nodeId,
        phyData->macInterfaceIndex,
        antennaModelInput,
        "ANTENNA-PATTERN-NUM-PATTERNS",
        &wasFound,
        &numPatterns);

    if (!wasFound)
    {
        ERROR_ReportError(
        "ANTENNA-PATTERN-NUM-PATTERNS is missing for the antenna"
        " model.\n");
        //exit from here

    }
    else
    {
        ERROR_Assert(numPatterns > 0 ,
            "Illegal number of patterns.\n");
        antennaPatterns->numOfPatterns = numPatterns;
    }

    IO_ReadInt(
        node,
        node->nodeId,
        phyData->macInterfaceIndex,
        antennaModelInput,
        "ANTENNA-PATTERN-AZIMUTH-RESOLUTION",
        &wasFound,
        &antennaPatterns->azimuthResolution);
    if (!wasFound)
    {
        antennaPatterns->azimuthResolution = 360;
    }

    IO_ReadInt(
        node,
        node->nodeId,
        phyData->macInterfaceIndex,
        antennaModelInput,
        "ANTENNA-PATTERN-ELEVATION-RESOLUTION",
        &wasFound,
        &antennaPatterns->elevationResolution);
    if (!wasFound)
    {
        antennaPatterns->elevationResolution = 180;
    }

    antennaPatterns->maxAzimuthIndex = antennaPatterns->azimuthResolution;
    antennaPatterns->maxElevationIndex = antennaPatterns->
        elevationResolution;

    IO_ReadCachedFile(
        node,
        node->nodeId,
        phyData->macInterfaceIndex,
        antennaModelInput,
        "ANTENNA-PATTERN-PATTERN-FILE",
        &wasFound,
        &antennaInput);

        if (!wasFound && antennaPatterns->antennaPatternType
        == ANTENNA_PATTERN_ASCII3D)
        {
            ERROR_ReportError(
                "ANTENNA-PATTERN-PATTERN-FILE is missing for the antenna"
                " model.\n");
        }

    if (wasFound && antennaPatterns->antennaPatternType
        == ANTENNA_PATTERN_ASCII3D)
    {
        antennaPatterns->is3DArray = TRUE;
        antennaPatterns->is3DGeometry = TRUE;
        strcpy(antennaPatterns->antennaFileName, antennaInput.ourName);
        ANTENNA_Read3DAsciiPatterns(node,phyIndex,&antennaInput,
            antennaPatterns);
        float*** element =
            (float***) antennaPatterns->antennaPatternElements;

        for (i = 0; i < antennaPatterns->numOfPatterns; i++)
        {
            for (j = 0;j <= antennaPatterns->elevationResolution; j++)
            {
                for (k = 0; k <= antennaPatterns->azimuthResolution; k++)
                {
                    element[i][j][k] = conversionParameter
                        - element[i][j][k];
                }
            }
        }

        antennaPatterns->boreSightAngle = (Orientation*) MEM_malloc(
            antennaPatterns->numOfPatterns * sizeof(Orientation));
        ERROR_Assert(antennaPatterns->boreSightAngle ,
            "memory allocation problem"
            "for antennaPatterns->boreSightAngle.\n");
        memset(antennaPatterns->boreSightAngle, 0,sizeof(Orientation));

        antennaPatterns->boreSightGain_dBi = (float*)
            MEM_malloc(antennaPatterns->numOfPatterns * sizeof(float));
        ERROR_Assert(antennaPatterns->boreSightGain_dBi ,
            "memory allocation problem for"
            "antennaPatterns->boreSightGain_dBi.\n");
        memset(antennaPatterns->boreSightGain_dBi, 0,sizeof(float));

        for (i = 0; i < antennaPatterns->numOfPatterns; i++)
        {
            highestGain = ANTENNA_LOWEST_GAIN_dBi;
            for (j = 0;j <= antennaPatterns->elevationResolution; j++)
            {
                for (k = 0; k <= antennaPatterns->azimuthResolution; k++)
                {
                    if (element[i][j][k] > highestGain)
                    {
                        highestGain = element[i][j][k];
                        highestazi = (float) k;
                        highestele = (float) j;
                    }
                }
            }
            highestazi = ((float)360 / antennaPatterns->
                azimuthResolution) * highestazi;
            antennaPatterns->boreSightAngle[i].azimuth = (short)
                highestazi;
            highestele = ((float)180 / (float)antennaPatterns->
                elevationResolution) * highestele;
            antennaPatterns->boreSightAngle[i].elevation = (short) (
                highestele - ANGLE_RESOLUTION/4);
            antennaPatterns->boreSightGain_dBi[i] = (float) highestGain;

        }

        if (DEBUG_ASCII3D)
        {
            for (i = 0; i < antennaPatterns->numOfPatterns; i++)
            {
                printf("\n 3D pattern is for pattern index %d \n", i);
                printf("Elevation \t\t Azimuth \t\t Gain\n");

                for (j = 0;j <= antennaPatterns->elevationResolution; j++)
                {
                    for (k = 0;k <= antennaPatterns->azimuthResolution; k++)
                    {
                        printf(" %3.6f \t\t %3.6f \t\t %3.6f \n",
                            ((float)180 / (float)antennaPatterns->
                            elevationResolution) * j,
                            ((float)360 / antennaPatterns->
                            azimuthResolution) * k,
                            element[i][j][k]);
                    }
                }
            }
        }
        return;
    }

    antennaPatterns->is3DArray = FALSE;
    antennaPatterns->is3DGeometry = TRUE;

    IO_ReadCachedFile(
        node,
        node->nodeId,
        phyData->macInterfaceIndex,
        antennaModelInput,
        "ANTENNA-AZIMUTH-PATTERN-FILE",
        &wasFound,
        &antennaInput);

    if (!wasFound)
    {
        sprintf(err,
            "Error: ANTENNA-AZIMUTH-PATTERN-FILE "
            "is missing for the node with nodeId %d\n",
            node->nodeId);
        ERROR_ReportError(err);

    }

    strcpy(antennaPatterns->antennaFileName, antennaInput.ourName);

    IO_ReadCachedFile(
        node,
        node->nodeId,
        phyData->macInterfaceIndex,
        antennaModelInput,
        "ANTENNA-ELEVATION-PATTERN-FILE",
        &wasFound1,
        &antennaInput1);

    ANTENNA_Read2DAsciiPatterns(node,phyIndex,&antennaInput,
        antennaPatterns,TRUE, conversionParameter);
    antennaPatterns->boreSightAngle = (Orientation*)
        MEM_malloc(numPatterns *sizeof(Orientation));
    ERROR_Assert(antennaPatterns->boreSightAngle ,
        "memory allocation problem for boreSightAngle.\n");
    memset(antennaPatterns->boreSightAngle, 0,sizeof(Orientation));

    antennaPatterns->boreSightGain_dBi =
        (float*) MEM_malloc(numPatterns *  sizeof(float));
    ERROR_Assert(antennaPatterns->boreSightGain_dBi ,
        "memory allocation problem for boreSightGain_dBi.\n");
    memset(antennaPatterns->boreSightGain_dBi, 0,sizeof(float));

    AntennaPatternElement** elements = (AntennaPatternElement**)
        antennaPatterns->antennaPatternElements;

    for (i = 0; i < numPatterns; i++)
    {
        highestGain = ANTENNA_LOWEST_GAIN_dBi;
        highestGainAngle = 0;
        for (j = 0; j <= antennaPatterns->azimuthResolution; j++)
        {
            if (DEBUG_ASCII2D)
            {
                printf("%f\t\t %f\n",
                    ((float)360/(antennaPatterns->
                    azimuthResolution))*j,
                    elements[AZIMUTH_INDEX][i].gains[j]);
            }
            if (elements[AZIMUTH_INDEX][i].gains[j] > highestGain)
            {
                highestGain = elements[AZIMUTH_INDEX][i].gains[j];
                highestGainAngle = ((float)360/(antennaPatterns->
                    azimuthResolution))*j;
            }
        }
        ERROR_Assert(highestGain != ANTENNA_LOWEST_GAIN_dBi ,
            "highest gain is ANTENNA_LOWEST_GAIN_dBi.\n");

        antennaPatterns->boreSightAngle[i].azimuth = (short)
            highestGainAngle;
        antennaPatterns->boreSightGain_dBi[i] = (float) highestGain;
    }

    if (!wasFound1)
    {
        for (i = 0; i < numPatterns; i++)
        {
            antennaPatterns->boreSightAngle[i].elevation = 0;
        }
        return;
    }

    ANTENNA_Read2DAsciiPatterns(node,phyIndex,
        &antennaInput1, antennaPatterns,FALSE, conversionParameter);

    for (i = 0; i < numPatterns; i++)
    {
        highestGain = ANTENNA_LOWEST_GAIN_dBi;
        highestGainAngle = 0;

        for (j = 0; j <= antennaPatterns->elevationResolution; j++)
        {
            if (DEBUG_ASCII2D)
            {
                printf(" %f\t\t gain%f\n",
                    ((float)180 / (float)antennaPatterns->
                    elevationResolution) * j,
                    elements[ELEVATION_INDEX][i].gains[j]);
            }

            if (elements[ELEVATION_INDEX][i].gains[j] > highestGain)
            {
                highestGain = elements[ELEVATION_INDEX][i].gains[j];
                highestGainAngle = ((float)180 / (float)antennaPatterns->
                    elevationResolution) * j;
            }
        }
        ERROR_Assert(highestGain != ANTENNA_LOWEST_GAIN_dBi ,
            "highest gain is ANTENNA_LOWEST_GAIN_dBi.\n");
        antennaPatterns->boreSightAngle[i].elevation = (short) (
            highestGainAngle - ANGLE_RESOLUTION/4);
        antennaPatterns->boreSightGain_dBi[i] += (float) highestGain;
    }

}//function end


// /**
// FUNCTION :: ANTENNA_MakeAntennaModelInput
// LAYER :: Physical Layer.
// PURPOSE :: Reads the antenna configuration parameters into
//            the NodeInput structure.
// PARAMETERS ::
// + node : Node* : node being used
//                  physical to be initialized
// + buf : char* : Path to input file.
// RETURN :: NodeInput * : pointer to nodeInput structure
// **/

NodeInput* ANTENNA_MakeAntennaModelInput(
           const NodeInput* nodeInput,
           char* buf)
{
    char* string;
    BOOL found = FALSE;

    int i;
    BOOL retVal;
    char inputStr[MAX_INPUT_FILE_LINE_LENGTH];
    NodeInput antennaInput;
    NodeInput* antennaModelInput = IO_CreateNodeInput ();

    antennaModelInput->numLines = 0;
    antennaModelInput->maxNumLines = MAX_ANTENNA_NUM_LINES;
    antennaModelInput->numFiles = 0;

    IO_ReadCachedFile(ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "ANTENNA-MODEL-CONFIG-FILE",
        &retVal,
        &antennaInput);

    if (retVal == FALSE)
    {
        ERROR_ReportError("ANTENNA-MODEL specified but "
            "ANTENNA-MODEL-CONFIG-FILE was not found.\n");
    }

    antennaModelInput->inputStrings =
        (char **) MEM_malloc(antennaModelInput->maxNumLines
        * sizeof(char *));
    memset(antennaModelInput->inputStrings, 0,
        antennaModelInput->maxNumLines * sizeof(char *));

    antennaModelInput->qualifiers =
        (char **) MEM_malloc(antennaModelInput->maxNumLines
        * sizeof(char *));
    memset(antennaModelInput->qualifiers, 0,
        antennaModelInput->maxNumLines * sizeof(char *));

    antennaModelInput->variableNames =
        (char **) MEM_malloc(antennaModelInput->maxNumLines
        * sizeof(char *));
    memset(antennaModelInput->variableNames, 0,
        antennaModelInput->maxNumLines * sizeof(char *));

    antennaModelInput->values =
        (char **) MEM_malloc(antennaModelInput->maxNumLines
        * sizeof(char *));
    memset(antennaModelInput->values, 0,
        antennaModelInput->maxNumLines * sizeof(char *));

    antennaModelInput->instanceIds =
        (int *) MEM_malloc(antennaModelInput->maxNumLines
        * sizeof(int));
    memset(antennaModelInput->instanceIds, 0,
        antennaModelInput->maxNumLines * sizeof(int));

    for (i = 0; i < antennaInput.numLines; i++)
    {
        int retVal;
        char variableStr[MAX_INPUT_FILE_LINE_LENGTH];
        char valueStr[MAX_INPUT_FILE_LINE_LENGTH];

        retVal = sscanf(antennaInput.inputStrings[i],
            "%s %s", variableStr, valueStr);
        ERROR_Assert(retVal == 2, "Format error in antenna model file\n");

        if (strcmp(variableStr, "ANTENNA-MODEL-NAME") == 0) {

            // If we previously already seen "ANTENNA-MODEL" statement
            // before, then that means we are done with this antenna
            // configuration.
            if (found)
            {
                break;
            }

            if (strcmp(buf, valueStr))
            {
                // Not the model we are looking for, so keep searching.
                continue;
            }
            else
            {
                // Found antenna model, so now get ready to read the its
                // configuration parameters.
                found = TRUE;
            }
        }
        else if (!found)
        {
            continue;
        }

        sprintf(inputStr, "%s %s", variableStr, valueStr);

        antennaModelInput->inputStrings[antennaModelInput->numLines]
            = strdup(inputStr);

        antennaModelInput->qualifiers[antennaModelInput->numLines] = NULL;
        antennaModelInput->variableNames[antennaModelInput->numLines]
            = NULL;
        antennaModelInput->instanceIds[antennaModelInput->numLines] = 0;
        antennaModelInput->values[antennaModelInput->numLines] = NULL;

        string =
            antennaModelInput->inputStrings[antennaModelInput->numLines];

        antennaModelInput->variableNames[antennaModelInput->numLines]
            = strdup(variableStr);

        antennaModelInput->values[antennaModelInput->numLines]
            = strdup(valueStr);

        ERROR_Assert(antennaModelInput->
            variableNames[antennaModelInput->numLines] != NULL,
            "Not enough memory to allocate string.\n");

        ERROR_Assert(antennaModelInput->values
            [antennaModelInput->numLines] != NULL,
            "Not enough memory to allocate string.\n");

        // If key has "-FILE" at the end and currently
        // reading only "-FILE" parameters,
        // load the secondary input file.

        if (strlen(antennaModelInput->variableNames[antennaModelInput->
            numLines]) > 5 && strcmp(IO_Right(antennaModelInput->
            variableNames[antennaModelInput->numLines], 5), "-FILE") == 0)
        {
            char *path = antennaModelInput->values[antennaModelInput->
                numLines];
            int i;

            // see if we already cached this file.
            for (i = 0; i < antennaModelInput->numFiles; i++)
            {
                if (strcmp(path, antennaModelInput->cachedFilenames[i]) == 0)
                {
                    // Secondary input file already cached.

                    break;
                }
            }

            if (i == antennaModelInput->numFiles)
            {
                NodeInput *nodeInput2 = IO_CreateNodeInput ();

                // Secondary input file not in cache.

                IO_CacheFile(nodeInput2, path);

                antennaModelInput->cachedFilenames[i]
                    = (char *) MEM_malloc(strlen(path) + 1);
                strcpy(antennaModelInput->cachedFilenames[i], path);

                antennaModelInput->cached[i] = nodeInput2;
                antennaModelInput->numFiles++;
            }

            ERROR_Assert(
                antennaModelInput->numFiles <= MAX_NUM_CACHED_FILES,
                "Too many input files.\n");
        }
        antennaModelInput->numLines++;
    }

#ifdef _WIN32
    #define snprintf _snprintf
#endif

    char errorStr[MAX_STRING_LENGTH];
    if (!found)
    {
        snprintf (errorStr, MAX_STRING_LENGTH,
            "Unable to find an ANTENNA-MODEL with name '%s'.\n", buf);
        ERROR_Assert(found, errorStr);
    }

    return antennaModelInput;
}

