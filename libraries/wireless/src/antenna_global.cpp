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

#include "api.h"
#include "partition.h"
#include "antenna.h"
#include "antenna_global.h"
#include "antenna_switched.h"
#include "antenna_steerable.h"
#include "antenna_patterned.h"
#if ADDON_EBE
#include "antenna_patterned-ebe.h"
#endif


#define DEBUG_GLOBAL 0

///------------------------------------------------------------
// INTERNAL UTILITIES THAT WE WANT INLINED
///------------------------------------------------------------

// /**
// FUNCTION :: ANTENNA_GlobalAntennaModelAlloc
// LAYER :: Physical Layer.
// PURPOSE :: Alloc a new model.
// PARAMETERS ::
// + partitionData : PartitionData* : Pointer to partition data.
// RETURN :: AntennaModelGlobal* : Pointer to the global
//                                 antenna model structure.
// **/

AntennaModelGlobal* ANTENNA_GlobalAntennaModelAlloc(
                    PartitionData* partitionData)
{
    AntennaModelGlobal* antennaModel =
        (AntennaModelGlobal *) MEM_malloc(sizeof(AntennaModelGlobal));
    ERROR_Assert(antennaModel ,
        "memory allocation problem for antennaModel.\n");
    memset(antennaModel, 0, sizeof(AntennaModelGlobal));

    return antennaModel;
}


// /**
// FUNCTION :: ANTENNA_GlobalAntennaModelPreInitialize
// LAYER :: Physical Layer.
// PURPOSE :: Preinitalize the global antenna structs.
// PARAMETERS ::
// + partitionData : PartitionData* : Pointer to partition data.
// RETURN :: void : NULL
// **/

void ANTENNA_GlobalAntennaModelPreInitialize(
     PartitionData* partitionData)
{
    //  Initalize global Antenna Model Structure
    partitionData->antennaModels =
        (AntennaModelGlobal *) MEM_malloc(
        MAX_ANTENNA_MODELS * sizeof(AntennaModelGlobal));
    ERROR_Assert(partitionData->antennaModels ,
        "memory allocation problem for the total"
            "number of antenna models.\n");
    memset(partitionData->antennaModels, 0,
        MAX_ANTENNA_MODELS * sizeof(AntennaModelGlobal));

    partitionData->numAntennaModels = 0;
}


// /**
// FUNCTION :: ANTENNA_GlobalAntennaModelInit
// LAYER :: Physical Layer.
// PURPOSE :: Reads the antenna configuration parameters
//            into the global antenna model structure.
// PARAMETERS ::
// + node : Node* : node being used.
// + phyIndex : int : interface for which physical to be
//                    initialized.
// + antennaModelInput : const NodeInput* : structure containing
//                                          contents of input
//                                          file
// + antennaModelName  : const char * : contains the name of the antenna
//                                      model to be initialized.
// RETURN :: void : NULL
// **/

void ANTENNA_GlobalAntennaModelInit(
     Node* node,
     int  phyIndex,
     const NodeInput* antennaModelInput,
     const char* antennaModelName)
{
    char    buf[MAX_STRING_LENGTH];
    BOOL    wasFound;

    PhyData *phyData = node->phyData[phyIndex];

    // The following 4 variables are declared to calculate symtessLoss.
    double antennaEfficiency;
    double antennaMismatchLoss_dB;
    double antennaConnectionLoss_dB;
    double antennaCableLoss_dB;
    float height;
    float antennaGain_dB;

    // Max number of global models reached
    ERROR_Assert(node->partitionData->numAntennaModels <
                 MAX_ANTENNA_MODELS ,
                    "numAntennaModels exceeded the limit.\n");

    // Get new model
    AntennaModelGlobal* antennaModel =
        &node->partitionData->antennaModels[
        node->partitionData->numAntennaModels];

    // Read in antenna model
    // Model name initialization with
    // ANTENNA-MODEL (required)

    strcpy(antennaModel->antennaModelName, antennaModelName);

    IO_ReadString(
        node,
        node->nodeId,
        phyData->macInterfaceIndex,
        antennaModelInput,
        "ANTENNA-MODEL-TYPE",
        &wasFound,
        buf);

    if (!wasFound)
    {
        ERROR_ReportError(
            "ANTENNA-MODEL-TYPE is missing for the antenna model.\n");
    }
    if (strcmp(buf, "OMNIDIRECTIONAL") == 0)
    {
        antennaModel->antennaModelType = ANTENNA_OMNIDIRECTIONAL;
    }
    else if (strcmp(buf, "SWITCHED-BEAM") == 0)
    {
        antennaModel->antennaModelType = ANTENNA_SWITCHED_BEAM;
    }
    else if (strcmp(buf, "STEERABLE") == 0)
    {
        antennaModel->antennaModelType = ANTENNA_STEERABLE;
    }
    else if (strcmp(buf, "PATTERNED") == 0)
    {
        antennaModel->antennaModelType = ANTENNA_PATTERNED;
    }
    else
    {
        char err[MAX_STRING_LENGTH];
        sprintf(err, "Unknown ANTENNA-MODEL-TYPE %s.\n", buf);
        ERROR_ReportError(err);
    }

    IO_ReadFloat(
        node,
        node->nodeId,
        phyData->macInterfaceIndex,
        antennaModelInput,
        "ANTENNA-HEIGHT",
        &wasFound,
        &height);


    if (wasFound)
    {
        ERROR_Assert(height >= 0 ,
            "Illegal height given in the file.\n");
        antennaModel->height = (float) height;
    }
    else
    {
        antennaModel->height = ANTENNA_DEFAULT_HEIGHT;
    }

    IO_ReadFloat(
        node,
        node->nodeId,
        phyData->macInterfaceIndex,
        antennaModelInput,
        "ANTENNA-GAIN",
        &wasFound,
        &antennaGain_dB);

    if (wasFound)
    {
        antennaModel->antennaGain_dB = (float) antennaGain_dB;
    }
    else
    {
        antennaModel->antennaGain_dB = ANTENNA_DEFAULT_GAIN_dBi;
    }

    // Set ANTENNA-EFFICIENCY

    IO_ReadDouble(
        node,
        node->nodeId,
        phyData->macInterfaceIndex,
        antennaModelInput,
        "ANTENNA-EFFICIENCY",
        &wasFound,
        &antennaEfficiency);

    if (wasFound)
    {
        antennaModel->antennaEfficiency_dB = -IN_DB(antennaEfficiency);
    }
    else
    {
        antennaModel->antennaEfficiency_dB = -IN_DB(ANTENNA_DEFAULT_EFFICIENCY);
    }

    // Set ANTENNA-MISMATCH-LOSS

    IO_ReadDouble(
        node,
        node->nodeId,
        phyData->macInterfaceIndex,
        antennaModelInput,
        "ANTENNA-MISMATCH-LOSS",
        &wasFound,
        &antennaMismatchLoss_dB);

    if (wasFound)
    {
        antennaModel->antennaMismatchLoss_dB = antennaMismatchLoss_dB;
    }
    else
    {
        antennaModel->antennaMismatchLoss_dB = ANTENNA_DEFAULT_MISMATCH_LOSS_dB;
    }

    // Set ANTENNA-CABLE-LOSS

    IO_ReadDouble(
        node,
        node->nodeId,
        phyData->macInterfaceIndex,
        antennaModelInput,
        "ANTENNA-CABLE-LOSS",
        &wasFound,
        &antennaCableLoss_dB);

    if (wasFound)
    {
        antennaModel->antennaCableLoss_dB = antennaCableLoss_dB;
    }
    else
    {
        antennaModel->antennaCableLoss_dB = ANTENNA_DEFAULT_CABLE_LOSS_dB;
    }

    // Set ANTENNA-CONNECTION-LOSS

    IO_ReadDouble(
        node,
        node->nodeId,
        phyData->macInterfaceIndex,
        antennaModelInput,
        "ANTENNA-CONNECTION-LOSS",
        &wasFound,
        &antennaConnectionLoss_dB);

    if (wasFound)
    {
        antennaModel->antennaConnectionLoss_dB = antennaConnectionLoss_dB;
    }
    else
    {
        antennaModel->antennaConnectionLoss_dB = ANTENNA_DEFAULT_CONNECTION_LOSS_dB;
    }

    if (DEBUG_GLOBAL)
    {
       printf("ANTENNA MODEL %s\n",antennaModel->antennaModelName);
       printf("HEIGHT %f\tGAIN %f\tEfficiency %f\n"
              "LOSSES %f %f %f",
       antennaModel->height, antennaModel->antennaGain_dB,
       antennaModel->antennaEfficiency_dB,
       antennaModel->antennaMismatchLoss_dB,
       antennaModel->antennaCableLoss_dB,
       antennaModel->antennaConnectionLoss_dB);
    }

    //This function is used to assign global radiation pattern for
    // each antenna.

    if (antennaModel->antennaModelType != ANTENNA_OMNIDIRECTIONAL)
    {
        antennaModel->antennaPatterns =
            ANTENNA_GlobalModelAssignPattern(node,
            phyIndex,
            antennaModelInput);
    }
    node->partitionData->numAntennaModels++;
    return;
}


// /**
// FUNCTION :: ANTENNA_GlobalAntennaModelGet
// LAYER :: Physical Layer.
// PURPOSE :: Return the model based on the name.
// PARAMETERS ::
// + partitionData : PartitionData* : Pointer to partition data.
// + antennaModelName  : const char* : contains the name of the
//                                     antenna model.
// RETURN :: AntennaModelGlobal* : Pointer to the global
//                                 antenna model structure.
// **/

AntennaModelGlobal* ANTENNA_GlobalAntennaModelGet(
                    PartitionData* partitionData,
                    const char* antennaModelName)
{
    AntennaModelGlobal* antennaModels = partitionData->antennaModels;
    int numModels = partitionData->numAntennaModels;
    int i;

    for (i = 0; i < numModels; i++)
    {
        if (strcmp(antennaModels[i].antennaModelName,
            antennaModelName) == 0)
        {
            return &antennaModels[i];
        }
    }
    return NULL;
}


// /**
// FUNCTION :: ANTENNA_GlobalModelAssignPattern
// LAYER :: Physical Layer.
// PURPOSE :: used to assign global radiation pattern for each
//            antenna.
// PARAMETERS ::
// + node : Node* : node being used.
// + phyIndex : int : interface for which physical to be
//                   initialized.
// + antennaModelInput : const NodeInput* : structure containing
//                                          contents of input
//                                          file
// RETURN :: AntennaPattern* : Pointer to the global
//                             antenna pattern structure.
// **/

AntennaPattern* ANTENNA_GlobalModelAssignPattern(
                Node* node,
                int phyIndex,
                const NodeInput* antennaModelInput)
  {
    AntennaPattern* antennaPatterns = NULL;
    char antennaPatternName[2 * MAX_STRING_LENGTH];
    BOOL wasFound = FALSE;
    PhyData *phyData = node->phyData[phyIndex];

        char    buf[MAX_STRING_LENGTH];
        wasFound = FALSE;
        IO_ReadString(
            node,
            node->nodeId,
            phyData->macInterfaceIndex,
            antennaModelInput,
            "ANTENNA-PATTERN-TYPE",
            &wasFound,
            buf);
            if (wasFound)
            {
                ANTENNA_GeneratePatterName(node,
                                           phyIndex,
                                           antennaModelInput,
                                           buf,
                                           antennaPatternName);
            }
    

    // find the global antenna structure for this type
    antennaPatterns =
        ANTENNA_GlobalAntennaPatternGet(node->partitionData,
        antennaPatternName);

    // Create a new global antenna structure
    if (antennaPatterns == NULL)
    {
        ANTENNA_GlobalAntennaPatternInit(node,
            phyIndex ,antennaModelInput,antennaPatternName);

        antennaPatterns =
            ANTENNA_GlobalAntennaPatternGet(node->partitionData,
            antennaPatternName);
    }
    else
    {
        char    buf[MAX_STRING_LENGTH];
        NodeInput antennaInput;

        IO_ReadString(
            node,
            node->nodeId,
            phyData->macInterfaceIndex,
            antennaModelInput,
            "ANTENNA-PATTERN-TYPE",
            &wasFound,
            buf);

        if (!wasFound)
        {
            return antennaPatterns;
        }

        if (strcmp(buf, "ASCII3D") == 0 ||
            strcmp(buf, "NSMA") == 0)
        {
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
                return antennaPatterns;
            }
            if (strcmp(antennaInput.ourName,
                       antennaPatterns->antennaFileName) != 0)
            {
                ERROR_ReportWarning( "Same Pattern name found for different"
                                      " antenna azimuth files");
            }
        }
        else if (strcmp(buf, "ASCII2D") == 0 ||
                 strcmp(buf, "TRADITIONAL") == 0)
        {
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
                return antennaPatterns;
            }

            if (strcmp(antennaInput.ourName,
                       antennaPatterns->antennaFileName) != 0)
            {
                ERROR_ReportWarning( "Same Pattern name found for different"
                                      " antenna azimuth files");
            }
        }
#if ADDON_EBE
        else if (strcmp(buf, "EBE") == 0)
        {
            // SNT ToDo
        }
#endif

#if ALE_ASAPS_LIB
        else if (strcmp(buf, "ASAPS") == 0)
        {
            ERROR_ReportError(
                    "ASAPS Antenna support hasn't been implemented.");
        }
#endif

        else
        {
            char err[MAX_STRING_LENGTH];
            sprintf(err,
                "Unknown ANTENNA-PATTERN-TYPE '%s' on node %d.\n",
                buf, node->nodeId);
            ERROR_ReportError(err);
        }

    }

    ERROR_Assert(antennaPatterns ,
        "unable to get antennaPatterns.\n");
    return antennaPatterns;
}


// /**
// FUNCTION :: ANTENNA_GlobalAntennaPatternGet
// LAYER :: Physical Layer.
// PURPOSE :: Return the antenna pattern based on the name.
// PARAMETERS ::
// + partitionData : PartitionData* : Pointer to partition data.
// + antennaPatternName  : const char* : contains the name of the
//                                       antenna pattern.
// RETURN :: AntennaPattern* : Pointer to the global
//                             antenna pattern structure.
// **/

AntennaPattern* ANTENNA_GlobalAntennaPatternGet(
                PartitionData* partitionData,
                const char* antennaPatternName)
 {
    AntennaPattern* antennaPatterns = partitionData->antennaPatterns;
    int numPatterns = partitionData->numAntennaPatterns;
    int i;

    for (i = 0; i < numPatterns; i++)
        {
            if (strcmp(antennaPatterns[i].antennaPatternName,
                antennaPatternName) == 0)
                {
                    return &antennaPatterns[i];
                }
        }

    return NULL;
 }


// /**
// FUNCTION :: ANTENNA_GlobalAntennaPatternInit
// LAYER :: Physical Layer.
// PURPOSE :: Init the antenna pattern structure for pattern name.
// PARAMETERS ::
// + node : Node* : node being used.
// + phyIndex : int : interface for which physical to be
//                    initialized.
// + antennaModelInput : const NodeInput* : structure containing
//                                          contents of input file.
// + antennaPatternName : const char* : antenna pattern name to be
//                                      initialized.
// RETURN :: Void : NULL
// **/

void ANTENNA_GlobalAntennaPatternInit(
     Node* node,
     int phyIndex,
     const NodeInput* antennaModelInput,
     const char* antennaPatternName)
{
    char    buf[MAX_STRING_LENGTH];
    BOOL    wasFound;

    PhyData *phyData = node->phyData[phyIndex];

    ERROR_Assert(node->partitionData->numAntennaPatterns <
                 MAX_ANTENNA_PATTERNS ,
                    "numAntennaPatterns exceeded the limit.\n");
    AntennaPattern* antennaPatterns =
        &node->partitionData->antennaPatterns[
        node->partitionData->numAntennaPatterns];
    strcpy(antennaPatterns->antennaPatternName , antennaPatternName);

    IO_ReadString(
        node,
        node->nodeId,
        phyData->macInterfaceIndex,
        antennaModelInput,
        "ANTENNA-PATTERN-TYPE",
        &wasFound,
        buf);

    if (!wasFound)
    {
        ERROR_ReportError(
            "ANTENNA-PATTERN-TYPE is missing for the antenna model.\n");
    }

    // Assign pattern
    if (strcmp(buf, "ASCII2D") == 0)
    {
        antennaPatterns->antennaPatternType = ANTENNA_PATTERN_ASCII2D;
        ANTENNA_ReturnAsciiPatternFile(node, phyIndex,antennaModelInput,
            antennaPatterns);
    }

    else if (strcmp(buf, "ASCII3D") == 0)
    {
        antennaPatterns->antennaPatternType = ANTENNA_PATTERN_ASCII3D;
        ANTENNA_ReturnAsciiPatternFile(node, phyIndex,antennaModelInput,
            antennaPatterns);
    }

    else if (strcmp(buf, "NSMA") == 0)
    {
        antennaPatterns->antennaPatternType = ANTENNA_PATTERN_NSMA;
        ANTENNA_ReturnNsmaPatternFile(node, phyIndex,antennaModelInput,
            antennaPatterns);
    }

    else if (strcmp(buf, "TRADITIONAL") == 0)
    {
        antennaPatterns->antennaPatternType = ANTENNA_PATTERN_TRADITIONAL;
        ANTENNA_ReturnTraditionalPatternFile(node,phyIndex,
            antennaModelInput,antennaPatterns);
    }
#if ADDON_EBE
    else if (strcmp(buf, "EBE") == 0)
    {
        // Fill the // memory of "antennaPatterns" based upon the settings
        // of the antennaModelInput file for this node's phy channel.
        antennaPatterns->antennaPatternType = ANTENNA_PATTERN_EBE;
        AntennaPatternedEbeFillPattern (node, phyIndex, antennaModelInput,
            antennaPatterns);
    }
#endif

#if ALE_ASAPS_LIB
    else if (strcmp(buf, "ASAPS") == 0)
    {
        ERROR_ReportError("ASAPS Antenna support hasn't been implemented.");
    }
#endif

    else
    {
        char err[MAX_STRING_LENGTH];
        sprintf(err,
            "Unknown ANTENNA-PATTERN-TYPE '%s' on node %d.\n",
            buf, node->nodeId);
        ERROR_ReportError(err);
    }

    node->partitionData->numAntennaPatterns++;
    return;
}


// /**
// FUNCTION :: ANTENNA_GlobalAntennaPatternInitFromConfigFile
// LAYER :: Physical Layer.
// PURPOSE :: Init the antenna pattern structure for pattern name for
//            the Old antenna model.
// PARAMETERS ::
// + node : Node* : node being used.
// + phyIndex : int : interface for which physical to be
//                    initialized.
// + antennaPatternName : const char* : antenna pattern name to be
//                                      initialized from the main
//                                      configuration file.
// + steer : BOOL : A boolean variable to differntiate which
//                  init function called this function steerable
//                  init or switched beam init.
// RETURN :: Void : NULL
// **/

void ANTENNA_GlobalAntennaPatternInitFromConfigFile(
     Node* node,
     int phyIndex,
     const char* antennaPatternName,
     BOOL steer)
 {
    NodeInput*  nodeInput = node->partitionData->nodeInput;

    ERROR_Assert(node->partitionData->numAntennaPatterns <
                 MAX_ANTENNA_PATTERNS ,
                    "numAntennaPatterns exceeded the limit.\n");
    AntennaPattern* antennaPatterns =
        &node->partitionData->antennaPatterns[
        node->partitionData->numAntennaPatterns];
    strcpy(antennaPatterns->antennaPatternName , antennaPatternName);

    antennaPatterns->antennaPatternType = ANTENNA_PATTERN_TRADITIONAL;
        ANTENNA_ReturnTraditionalPatternFile(node, phyIndex,nodeInput,
        antennaPatterns);

    node->partitionData->numAntennaPatterns++;
 }


// /**
// FUNCTION :: ANTENNA_GlobalAntennaPatternPreInitialize
// LAYER :: Physical Layer.
// PURPOSE :: Preinitalize the global antenna structs.
// PARAMETERS ::
// + partitionData : PartitionData* : Pointer to partition data.
// RETURN :: void : NULL
// **/

void ANTENNA_GlobalAntennaPatternPreInitialize(
     PartitionData* partitionData)
{
    //  Initalize global Antenna Model Structure
    partitionData->antennaPatterns =
        (AntennaPattern *) MEM_malloc(
        MAX_ANTENNA_PATTERNS * sizeof(AntennaPattern));
    ERROR_Assert(partitionData->antennaPatterns ,
        "Unable to preinitialize antennaPatterns.\n");
    memset(partitionData->antennaPatterns, 0,
        MAX_ANTENNA_PATTERNS * sizeof(AntennaPattern));

    partitionData->numAntennaPatterns = 0;
}

// FUNCTION :: ANTENNA_GeneratePatterName
// LAYER :: Physical Layer.
// PURPOSE :: Generate the Pattern name base on Pattern type.
// PARAMETERS ::
// + node : Node* : node being used.
// + phyIndex : int : interface for which physical to be
//                    initialized.
// + antennaModelInput : const NodeInput* : structure containing
//                                          contents of input
//                                          file
//+ patternType : char* patternType : Type of Pattern
// + antennaPatternName :char* : antenna pattern name to be
//                                      initialized from the main
//                                      configuration file.

// RETURN :: void : NULL
// **/


void ANTENNA_GeneratePatterName(Node* node,
                                int phyIndex,
                                const NodeInput* antennaModelInput,
                                const char* patternType,
                                char* antennaPatternName)
{
    BOOL wasFound = FALSE;
    PhyData *phyData = node->phyData[phyIndex];
    if ((strcmp(patternType, "ASCII2D") == 0 ||
          strcmp(patternType, "TRADITIONAL") == 0))
    {
        wasFound = FALSE;
        char elevPatternName[2*MAX_STRING_LENGTH];
        IO_ReadString(
            node,
            node->nodeId,
            phyData->macInterfaceIndex,
            antennaModelInput,
            "ANTENNA-AZIMUTH-PATTERN-FILE",
            &wasFound,
            antennaPatternName);
        if (!wasFound)
        {
            char FileError[MAX_STRING_LENGTH];
            sprintf(FileError, "Error: ANTENNA-AZIMUTH-PATTERN-FILE "
                    "is missing for the antenna with phy %d\n",
                    phyIndex);

            ERROR_ReportError(FileError);
        }
        wasFound = FALSE;
        IO_ReadString(
            node,
            node->nodeId,
            phyData->macInterfaceIndex,
            antennaModelInput,
            "ANTENNA-ELEVATION-PATTERN-FILE",
            &wasFound,
            elevPatternName);

        if (!wasFound)
        {
            strcpy(elevPatternName,"NONE");
        }
        //If Elevation Pattern name is bigger concat last n chars
        //from end of file name
        char* elevtempPatternName;
        elevtempPatternName = elevPatternName;
        int n = 2*MAX_STRING_LENGTH - strlen(antennaPatternName);
        int advance = strlen(elevPatternName) - n;
        if (advance > 0)
        {
            elevtempPatternName = elevPatternName + advance;

        }

        strncat(antennaPatternName,elevtempPatternName, n);
    }

    else if ((strcmp(patternType, "ASCII3D") == 0 ||
       strcmp(patternType, "NSMA") == 0))
    {
        wasFound = FALSE;
        IO_ReadString(
            node,
            node->nodeId,
            phyData->macInterfaceIndex,
            antennaModelInput,
            "ANTENNA-PATTERN-PATTERN-FILE",
            &wasFound,
            antennaPatternName);

    }
}

