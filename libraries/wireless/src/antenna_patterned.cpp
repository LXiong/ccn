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
#include "antenna.h"
#include "partition.h"
#include "antenna_patterned.h"
#include "coordinates.h"

#include "antenna_switched.h"
#include "antenna_steerable.h"
#if ADDON_EBE
#include "antenna_patterned-ebe.h"
#endif


// /**
// FUNCTION :: AntennaPatternedAlloc
// LAYER :: Physical Layer.
// PURPOSE :: Allocate new patterned struct.
// PARAMETERS :: NONE
// RETURN :: AntennaPatterned* : pointer to the structure of
//                               patterned antenna.
// **/

static
AntennaPatterned* AntennaPatternedAlloc(void)
{
    AntennaPatterned *antennaVars =
        (AntennaPatterned *) MEM_malloc(sizeof(AntennaPatterned));
    ERROR_Assert(antennaVars ,
        "unable to alloc patterned antenna.\n");
    memset(antennaVars, 0, sizeof(AntennaPatterned));

    return antennaVars;
}
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
     const NodeInput* nodeInput)
{

    PhyData* phyData = node->phyData[phyIndex];
    BOOL wasFound;
    char antennaPatternName[2 * MAX_STRING_LENGTH];
    float antennaGain_dB;
    float antennaHeight;

    AntennaPatterned* patterned = AntennaPatternedAlloc();
    phyData->antennaData = (AntennaModel* )MEM_malloc(sizeof(AntennaModel));
    memset(phyData->antennaData, 0, sizeof(AntennaModel));


    phyData->antennaData->antennaVar = patterned;
    phyData->antennaData->numModels++;
    phyData->antennaData->antennaModelType = ANTENNA_PATTERNED;


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
        patterned->antennaGain_dB = (float) antennaGain_dB;
    }
    else
    {
        patterned->antennaGain_dB = ANTENNA_DEFAULT_GAIN_dBi;
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
        patterned->antennaHeight = (float)antennaHeight;
    }
    else
    {
        patterned->antennaHeight = ANTENNA_DEFAULT_HEIGHT;

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
            patterned->pattern =
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
            patterned->pattern =
            ANTENNA_GlobalAntennaPatternGet(node->partitionData,
            antennaPatternName);

            // Create a new global antenna structure
            if (patterned->pattern == NULL)
            {
                ANTENNA_GlobalAntennaPatternInitFromConfigFile(node,
                    phyIndex, antennaPatternName,FALSE);
                patterned->pattern =
                    ANTENNA_GlobalAntennaPatternGet(node->partitionData,
                    antennaPatternName);
            }
        }


    ERROR_Assert(patterned->pattern != NULL,
                       "Antenna initialization error!\n");

    patterned->numPatterns =
        patterned->pattern->numOfPatterns;
    patterned->patternIndex = ANTENNA_DEFAULT_PATTERN;
    phyData->antennaData->antennaPatternType
        = patterned->pattern->antennaPatternType;
    return;
}


// /**
// FUNCTION :: ANTENNA_PatternedInit
// LAYER :: Physical Layer.
// PURPOSE :: initialization of patterned antenna.
// PARAMETERS ::
// + node: Node* : Pointer to Node.
//.+ nodeInput : const NodeInput* : pointer to node input
// + phyIndex: int : interface for which physical to be initialized
// + antennaModel : const AntennaModelGlobal* : pointer to global
//                                              antenna model
// RETURN :: void : NULL
// **/

void ANTENNA_PatternedInit(
     Node* node,
     const NodeInput* nodeInput,
     int  phyIndex,
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


    phyData->antennaData =
        (AntennaModel*) MEM_malloc(sizeof(AntennaModel));

    ERROR_Assert(phyData->antennaData ,
        "memory allocation problem for phyData->antennaData.\n");
    memset(phyData->antennaData, 0,sizeof(AntennaModel));

    // Assign antenna model based on Node's model type

    AntennaPatterned *antennaVars = AntennaPatternedAlloc();

    antennaVars->patternIndex = ANTENNA_PATTERN_NOT_SET;

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
        antennaVars->antennaGain_dB = (float)atof(buf);
    }
    else
    {
        antennaVars->antennaGain_dB
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
        antennaVars->antennaHeight = (float) atof(buf);
    }
    else
    {
        antennaVars->antennaHeight
            = antennaModel->height;
    }

    antennaVars->modelIndex = 0;

    antennaVars->numPatterns = antennaModel->antennaPatterns->numOfPatterns;
    antennaVars->pattern = antennaModel->antennaPatterns;
    antennaVars->patternIndex = ANTENNA_DEFAULT_PATTERN;

    // Assign antenna model based on Node's model type

    phyData->antennaData->antennaVar = antennaVars;
    phyData->antennaData->antennaModelType = antennaModel->antennaModelType;
    phyData->antennaData->numModels++;
    phyData->antennaData->antennaPatternType =
                    antennaVars->pattern->antennaPatternType;
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
// FUNCTION :: AntennaPatternedUsingDefaultPattern
// LAYER :: Physical Layer.
// PURPOSE :: Is using default pattern.
// PARAMETERS ::
// + phyData : PhyData* : pointer to the struct PhyData
// RETURN :: BOOL : Return TRUE if antenna pattern index
//                  is ANTENNA_DEFAULT_PATTERN
// **/

BOOL AntennaPatternedUsingDefaultPattern(
     PhyData* phyData)
{
    AntennaPatterned* patterned =
        (AntennaPatterned *)phyData->antennaData->antennaVar;

    ERROR_Assert(patterned->patternIndex != ANTENNA_PATTERN_NOT_SET ,
        "pattern index is ANTENNA_PATTERN_NOT_SET.\n");

    if (patterned->patternIndex == ANTENNA_DEFAULT_PATTERN)
     {
        return TRUE;
     }
    else
     {
        return FALSE;
     }
}


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
     Node* node,
     int phyIndex,
     int patternIndex)
{
    PhyData* phyData = node->phyData[phyIndex];
    AntennaPatterned* patterned =
        (AntennaPatterned *)phyData->antennaData->antennaVar;

    ERROR_Assert(phyData->antennaData->antennaModelType ==
        ANTENNA_PATTERNED ,"antennaModelType is not patterned.\n");
    ERROR_Assert(patternIndex < patterned->numPatterns ,
        "Illegal pattern index.\n");

    patterned->patternIndex = patternIndex;
}


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
     Node* node,
     int phyIndex,
     int* patternIndex)
{
    PhyData* phyData = node->phyData[phyIndex];
    AntennaPatterned* patterned =
        (AntennaPatterned *)phyData->antennaData->antennaVar;

    ERROR_Assert(phyData->antennaData->antennaModelType ==
        ANTENNA_PATTERNED ,
            "antennaModelType not patterned.\n");

    *patternIndex = patterned->patternIndex;

    return;
}


// /**
// FUNCTION :: AntennaPatternedReturnPatternIndex
// LAYER :: Physical Layer.
// PURPOSE :: Return the current pattern index.
// PARAMETERS ::
// + node : Node* : node being initialized.
// + phyIndex : int : interface for which physical to be
//                    initialized.
// RETURN :: int : Return the patternIndex
// **/

int AntennaPatternedReturnPatternIndex(
    Node* node,
    int phyIndex)
{
    PhyData* phyData = node->phyData[phyIndex];

    AntennaPatterned* patterned =
        (AntennaPatterned *)phyData->antennaData->antennaVar;

    return patterned->patternIndex;
}


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
// RETURN :: float : Return the antennaGain.
// **/

float AntennaPatternedGainForThisDirection(
      Node *node,
      int phyIndex,
      Orientation DOA)
{
    PhyData* phyData = node->phyData[phyIndex];

    AntennaPatterned* patterned =
        (AntennaPatterned *)phyData->antennaData->antennaVar;
    ERROR_Assert(patterned ,
        "unable to get the struct of patterned antenna.\n");

    ERROR_Assert(patterned->patternIndex != ANTENNA_PATTERN_NOT_SET ,
        "patterned index is ANTENNA_PATTERN_NOT_SET.\n");

    switch (patterned->pattern->antennaPatternType)
    {

#if ADDON_EBE
    case ANTENNA_PATTERN_EBE:
        {
            return AntennaPatternedEbeGainForThisDirection(node, phyIndex,
                patterned, DOA);
            break;
        }
#endif

#if ALE_ASAPS_LIB
    case ANTENNA_PATTERN_ASAPS:
    {
        ERROR_ReportError("ASAPS Antenna support hasn't been implemented.");
        return 0.0;
        break;
    }
#endif
    default:
        {
            return ANTENNA_PatternedGainForThisDirection(node,
                phyIndex, DOA);
            break;
        }
    }
}


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
// RETURN :: float : Return the antennaGain.
// **/

float AntennaPatternedGainForThisDirectionWithPatternIndex(
      Node* node,
      int phyIndex,
      int patternIndex,
      Orientation DOA)
{
    AntennaPatternedSetPattern(node, phyIndex, patternIndex);

    return AntennaPatternedGainForThisDirection(node, phyIndex, DOA);
}


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
// RETURN :: void
// **/

void AntennaPatternedMaxGainPatternForThisSignal(
     Node* node,
     int phyIndex,
     PropRxInfo* propRxInfo,
     int* patternIndex,
     float* gain_dBi)
{
    PhyData* phyData = node->phyData[phyIndex];
    AntennaPatterned* patterned =
        (AntennaPatterned *)phyData->antennaData->antennaVar;
    int i;
    int maxGainPattern = ANTENNA_OMNIDIRECTIONAL_PATTERN;
    float maxGain_dBi = ANTENNA_LOWEST_GAIN_dBi;
    int originalPatternIndex = patterned->patternIndex;
    int numPatterns = patterned->numPatterns;

    ERROR_Assert(phyData->antennaData->antennaModelType ==
        ANTENNA_PATTERNED ,
            "antennaModelType not patterned.\n");

    for (i = 0; i < numPatterns; i++)
    {
        float gain_dBi;

        AntennaPatternedSetPattern(node, phyIndex, i);
        gain_dBi =
            AntennaPatternedGainForThisDirection(node,
                phyIndex, propRxInfo->rxDOA);

        if (gain_dBi > maxGain_dBi)
        {
            maxGainPattern = i;
            maxGain_dBi = gain_dBi;
        }
    }

    *patternIndex = maxGainPattern;
    *gain_dBi = maxGain_dBi;

    patterned->patternIndex = originalPatternIndex;

    return;
}


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
     Node* node,
     int phyIndex,
     double azimuthAngle)
{

    PhyData* phyData = node->phyData[phyIndex];

    ERROR_Assert(phyData->antennaData->antennaModelType ==
        ANTENNA_PATTERNED ,"antenna model type is not"
            "patterned.\n");

    AntennaPatterned* patterned =
        (AntennaPatterned *)phyData->antennaData->antennaVar;

    ERROR_Assert(patterned->patternIndex != ANTENNA_PATTERN_NOT_SET ,
        "pattern index is ANTENNA_PATTERN_NOT_SET.\n");

    switch (patterned->pattern->antennaPatternType)
    {

#ifdef ADDON_EBE
    case ANTENNA_PATTERN_EBE:
        {
            AntennaPatternedEbeSetBestPatternForAzimuth(node, patterned,
                azimuthAngle);
            break;
        }
#endif

#ifdef ALE_ASAPS_LIB
    case ANTENNA_PATTERN_ASAPS:
    {
        ERROR_ReportError("ASAPS Antenna support hasn't been implemented.");
        break;
    }
#endif

    case ANTENNA_PATTERN_TRADITIONAL:
    case ANTENNA_PATTERN_ASCII2D:
    case ANTENNA_PATTERN_ASCII3D:
    case ANTENNA_PATTERN_NSMA:
        {
            ANTENNA_PatternedSetBestPatternForAzimuth(node, phyIndex,
                azimuthAngle);
            break;
        }
    default:
        {
            ERROR_ReportError("Undefined antenna pattern type.");
            break;
        }
    }

    //GuiStart
    if (node->guiOption)
       {
        GUI_SetPatternIndex(node,
                            phyData->macInterfaceIndex,
                            patterned->patternIndex,
                            node->getNodeTime());
       }
    //GuiEnd
}


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
     double azimuthAngle)
{
   PhyData* phyData = node->phyData[phyIndex];
   int i;
   AntennaPatterned* patterned =
    (AntennaPatterned *)phyData->antennaData->antennaVar;
   for (i = 0;i < patterned->numPatterns;i++)
   {
       if (patterned->pattern->boreSightAngle[i].azimuth == (short)
       azimuthAngle)
       {
             patterned->patternIndex = i;
             return;
       }
        if (((short)azimuthAngle > patterned->pattern->boreSightAngle[i].
        azimuth) &&
           ((short)azimuthAngle < patterned->pattern->boreSightAngle[i + 1].
           azimuth) && ((i + 1) <
            patterned->numPatterns))
       {
          patterned->patternIndex = i;
          return;
       }
   }
   patterned->patternIndex = patterned->numPatterns - 1;
   return;
}


// /**
// FUNCTION :: ANTENNA_PatternedGainForThisDirection
// LAYER :: Physical Layer.
// PURPOSE :: Return gain for a particular direction.
// PARAMETERS ::
// + node : Node* : node being initialized.
// + phyIndex : int : interface for which physical to be
//                    initialized.
// + DOA : Orientation : azimuth & elvation angle
// RETURN :: float : Return the antennaGain
// **/

float ANTENNA_PatternedGainForThisDirection(
      Node* node,
      int phyIndex,
      Orientation DOA)
{
    PhyData* phyData = node->phyData[phyIndex];

    Orientation nodeOrientation;
    Orientation orientation;

    float aziAngle;
    float eleAngle;
    int aziAngleIndex;
    int eleAngleIndex;
    float antennaMaxGain_dBi = ANTENNA_LOWEST_GAIN_dBi;

    AntennaPatterned* patterned =
        (AntennaPatterned *)phyData->antennaData->antennaVar;

    ERROR_Assert(patterned ,
        "unable to get the struct for patterned antenna.\n");

    ERROR_Assert(patterned->patternIndex != ANTENNA_PATTERN_NOT_SET ,
        "patterned index is ANTENNA_PATTERN_NOT_SET.\n");

    MOBILITY_ReturnOrientation(node, &nodeOrientation);

    orientation.azimuth = nodeOrientation.azimuth
                                + phyData->antennaMountingAngle.azimuth;
    orientation.elevation = nodeOrientation.elevation
                                + phyData->antennaMountingAngle.elevation;

    if (patterned->pattern->is3DArray)
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
        if (!(patterned->pattern->is3DGeometry))
        {
            // For Pattern cut Geometry
            if (aziAngle > 90 && aziAngle < 270)
            {
                eleAngle = ANGLE_RESOLUTION/2 - eleAngle;
                eleAngle = (float)
                    COORD_NormalizeElevationAngle ((int)eleAngle);
            }
        }


        aziAngleIndex = (int)(((float)patterned->pattern->
                            azimuthResolution / 360) * aziAngle);

        eleAngleIndex = (int)(((float) patterned->pattern->
                            elevationResolution / 180) *
                            (eleAngle + ANGLE_RESOLUTION/4));

        float*** image =
            (float***) patterned->pattern->antennaPatternElements;

        antennaMaxGain_dBi =
            image[patterned->patternIndex][eleAngleIndex][aziAngleIndex];

        return antennaMaxGain_dBi;
    }

    AntennaPatternElement **element;
    element = (AntennaPatternElement**) patterned->pattern->
    antennaPatternElements;

    aziAngle = (float) (DOA.azimuth - orientation.azimuth);
    aziAngle = (float) COORD_NormalizeAzimuthAngle ((int)aziAngle);

    eleAngle = (float) (DOA.elevation - orientation.elevation);
    eleAngle = (float)  COORD_NormalizeElevationAngle ((int)eleAngle);

    if (fabs(eleAngle) > 90 && element[PATTERNED_ELEVATION_INDEX])
    {
        aziAngle += ANGLE_RESOLUTION / 2;
        eleAngle = ANGLE_RESOLUTION/2 - eleAngle;
        aziAngle = (float) COORD_NormalizeAzimuthAngle ((int)aziAngle);
        eleAngle = (float) COORD_NormalizeElevationAngle ((int)eleAngle);
    }

    aziAngleIndex = (int)(((float)patterned->pattern->
        azimuthResolution / 360) * aziAngle);

    antennaMaxGain_dBi =
        element[PATTERNED_AZIMUTH_INDEX][patterned->patternIndex].gains
        [aziAngleIndex];

    if (element[PATTERNED_ELEVATION_INDEX] != NULL)
    {
        if (patterned->pattern->is3DGeometry)
        {
            eleAngleIndex = (int)(((float) patterned->pattern->
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
            eleAngleIndex = (int)(((float) patterned->pattern->
                        elevationResolution / 360) *
                        (eleAngle + ANGLE_RESOLUTION/2));
        }

        antennaMaxGain_dBi +=
            element[PATTERNED_ELEVATION_INDEX]
            [patterned->patternIndex].gains[eleAngleIndex];
    }
    return antennaMaxGain_dBi;
}

