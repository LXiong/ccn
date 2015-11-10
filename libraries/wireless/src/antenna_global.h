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
// PACKAGE     :: ANTENNA_GLOBAL
// DESCRIPTION :: This file describes additional data structures and functions used by antenna models.
// **/

#ifndef ANTENNA_GLOBAL_H
#define ANTENNA_GLOBAL_H

/*
# ANTENNA-EFFICIENCY                        <antenna efficiency in dB>
# ANTENNA-MISMATCH-LOSS                     <antenna mismatch loss in dB>
# ANTENNA-CABLE-LOSS                        <antenna cable loss in dB>
# ANTENNA-CONNECTION-LOSS
*/

#ifdef __cplusplus
extern "C" {
#endif

// /**
// CONSTANT :: MAX_ANTENNA_MODELS : 50
// DESCRIPTION :: Maximum number of models to allow.
// **/
#define  MAX_ANTENNA_MODELS     50

// /**
// CONSTANT :: MAX_ANTENNA_PATTERNS : 50
// DESCRIPTION :: Maximum number of antenna patterns to allow.
// **/
#define  MAX_ANTENNA_PATTERNS   50


// /**
// ENUM :: AntennaModelType
// DESCRIPTION :: Different types of antenna models supported.
// **/
enum AntennaModelType {
    ANTENNA_OMNIDIRECTIONAL,
    ANTENNA_SWITCHED_BEAM,
    ANTENNA_STEERABLE,
    ANTENNA_PATTERNED
};


// /**
// ENUM :: AntennaPatternType
// DESCRIPTION :: Different types of antenna pattern types supported.
// **/

enum AntennaPatternType {
    ANTENNA_PATTERN_TRADITIONAL,
    ANTENNA_PATTERN_ASCII2D,
    ANTENNA_PATTERN_ASCII3D,
    ANTENNA_PATTERN_NSMA,
    ANTENNA_PATTERN_EBE,
    ANTENNA_PATTERN_ASAPS
};

// /**
// ENUM :: NSMAPatternVersion
// DESCRIPTION :: Different types of NSMA pattern versions supported.
// **/

enum NSMAPatternVersion {
    NSMA_TRADITIONAL,
    NSMA_REVISED,
};
// /**
// ENUM :: AntennaGainUnit
// DESCRIPTION :: Different types of antenna gain units supported.
// **/

enum AntennaGainUnit {
    ANTENNA_GAIN_UNITS_DBI,
    ANTENNA_GAIN_UNITS_VOLT_METERS
};


// /**
// ENUM :: AntennaPatternUnit
// DESCRIPTION :: Different types of antenna pattern units supported.
// **/

enum AntennaPatternUnit {
    ANTENNA_PATTERN_UNITS_DEGREES_0_TO_90,
    ANTENNA_PATTERN_UNITS_DEGREES_0_TO_180,
    ANTENNA_PATTERN_UNITS_DEGREES_0_TO_360,
    ANTENNA_PATTERN_UNITS_DEGREES_MINUS_90_TO_90,
    ANTENNA_PATTERN_UNITS_DEGREES_MINUS_180_TO_180
};


typedef float AntennaElementGain;


// /**
// STRUCT :: struct_antenna_pattern_element
// DESCRIPTION :: Structure for antenna pattern elements
// **/

struct AntennaPatternElement {
    int                 numGainValues;  // Number of array items
    AntennaGainUnit     units;          // Units for gain

    AntennaElementGain  *gains;         // gain array pointer
};


// /**
// STRUCT :: struct_antenna_pattern
// DESCRIPTION :: Structure for antenna pattern
// **/

struct AntennaPattern {
    char                    antennaPatternName[2 * MAX_STRING_LENGTH];
    char                    antennaFileName[2 * MAX_STRING_LENGTH];
    AntennaPatternType      antennaPatternType; // Pattern Type

    int                     numOfPatterns;      // Number of patterns
    int                     patternSetRepeatAngle;//repeat angle for
                                                      //steerable antenna
    Orientation*             boreSightAngle;     // Orentation of bore
    float*                   boreSightGain_dBi;  // Gain at bore
    AntennaPatternUnit      azimuthUnits;       // Units of az
    AntennaPatternUnit      elevationUnits;     // Units of el
    int                 azimuthResolution;  // Resolution of az
                                                // units
    int                 elevationResolution;// Resolution of el
                                                // units
    int         maxAzimuthIndex;    // Maximum az
    int         maxElevationIndex;  // Maximum el
    bool        is3DGeometry;       // True if pattern is in 3D geometry
                                    // False if it is in pattern cut geometry
    bool        is3DArray;          // True if pattern is saved in 3D array
                                    // False if it is saved in 2D array
    void*                   antennaPatternElements; // Pattern array
};


// /**
// STRUCT :: struct_antenna_Global_model
// DESCRIPTION :: Structure for antenna model
// **/

struct AntennaModelGlobal {
    char                antennaModelName[MAX_STRING_LENGTH];
    AntennaModelType    antennaModelType;   // Model type
    float                   height;         // Height in meters
    float                   antennaGain_dB; // Max antenna
                                                //gain
    double                  antennaEfficiency_dB;
    double                  antennaMismatchLoss_dB;
    double                  antennaConnectionLoss_dB;
    double                  antennaCableLoss_dB;
    AntennaPattern      *antennaPatterns;   // Array of antenna patterns
};


// /**
// FUNCTION :: ANTENNA_GlobalAntennaModelPreInitialize
// LAYER :: Physical Layer.
// PURPOSE :: Preinitalize the global antenna structs.
// PARAMETERS ::
// + partitionData : PartitionData*  : Pointer to partition data.
// RETURN :: void : NULL
// **/

void ANTENNA_GlobalAntennaModelPreInitialize(
    PartitionData* partitionData);


// /**
// FUNCTION  :: ANTENNA_GlobalAntennaPatternPreInitialize
// LAYER :: Physical Layer.
// PURPOSE :: Preinitalize the global antenna structs.
// PARAMETERS ::
//  + partitionData : PartitionData*  : Pointer to partition data.
// RETURN :: void : NULL
// **/

void ANTENNA_GlobalAntennaPatternPreInitialize(
    PartitionData* partitionData);


// /**
// FUNCTION :: ANTENNA_GlobalModelAssignPattern
// LAYER :: Physical Layer.
// PURPOSE :: used to assign global radiation pattern for each
//            antenna.
// PARAMETERS ::
// + node : Node* : node being used.
// + phyIndex : int : interface for which physical to be
//                    initialized.
// + antennaModelInput : const NodeInput* : structure containing
//                                          contents of input
//                                          file
// RETURN :: AntennaPattern* : Pointer to the global
//                             antenna pattern structure.
// **/

AntennaPattern* ANTENNA_GlobalModelAssignPattern(
    Node* node,
    int phyIndex,
    const NodeInput* antennaModelInput);


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
// +antennaModelName  : const char * : contains the name of the antenna
//                                     model to be initialized.
// RETURN :: void : NULL
// **/

void ANTENNA_GlobalAntennaModelInit(
    Node* node,
    int  phyIndex,
    const NodeInput* antennaModelInput,
    const char* antennaModelName);

// /**
// FUNCTION :: ANTENNA_GlobalAntennaPatternInitFromConfigFile
// LAYER :: Physical Layer.
// PURPOSE :: Init the antenna pattern structure for pattern name for
//            the Old antenna model.
// PARAMETERS  ::
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
    BOOL steer);

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
    const char* antennaPatternName);


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
    PartitionData* partitionData);


// /**
// FUNCTION :: ANTENNA_GlobalAntennaModelGet
// LAYER :: Physical Layer.
// PURPOSE :: Return the model based on the name.
// PARAMETERS ::
// + partitionData : PartitionData* : Pointer to partition data.
// + antennaModelName : const char* : contains the name of the
//                                     antenna model.
// RETURN :: AntennaModelGlobal* : Pointer to the global
//                                 antenna model structure.
// **/

AntennaModelGlobal* ANTENNA_GlobalAntennaModelGet(
    PartitionData* partitionData,
    const char* antennaModelName);


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
    const char* antennaPatternName);

// /**
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
// + antennaPatternName : char* : antenna pattern name to be
//                                      initialized from the main
//                                      configuration file.
// RETURN :: void : NULL
// **/


void ANTENNA_GeneratePatterName(Node* node,
                                int phyIndex,
                                const NodeInput* antennaModelInput,
                                const char* patternType,
                                char* antennaPatternName);

#ifdef __cplusplus
}
#endif

#endif // ANTENNA_GLOBAL_H
