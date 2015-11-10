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

#include <iostream>
using namespace std;

#include "api.h"

#include "terrain_esri_shp_shared.h"

void
EsriShpInitBody(
    TerrainData* terrainData,
    NodeInput* nodeInput,
    int& numFiles,
    char**& filePaths,
    BOOL masterProcess)
{
    cout << endl;

    // Make one copy of the *hard-coded* default shapefile parameters.

    ShptoxmlData sxDataDefault;
    ShptoxmlInitData(sxDataDefault);

    // Set boundaries to QualNet terrain box.

    sxDataDefault.m_minUserLat = terrainData->getOrigin().latlonalt.latitude;
    sxDataDefault.m_maxUserLat
        = terrainData->getOrigin().latlonalt.latitude
        + terrainData->getDimensions().latlonalt.latitude;
    sxDataDefault.m_minUserLon = terrainData->getOrigin().latlonalt.longitude;
    sxDataDefault.m_maxUserLon
        = terrainData->getOrigin().latlonalt.longitude
        + terrainData->getDimensions().latlonalt.longitude;

    // Read shapefiles.

    int numShps;
    for (numShps = 0; ; numShps++)
    {
        ShptoxmlData sxData = sxDataDefault;
        bool retVal
            = EsriShpShpReadParameters(
                  terrainData, nodeInput, sxData, numShps);

        if (!retVal) { break; }

        // Only partition 0 creates the shape file
        // Other partitions read partition 0's output
        if (masterProcess)
        {
            ShptoxmlOpenShapefile(sxData);
            ShptoxmlReadShpHeader(sxData);

            if (sxData.m_hDbf != NULL)
            {
                ShptoxmlReadDbfFieldNames(sxData);
                ShptoxmlCheckShpDbfRecordCountConsistency(sxData);
            }

            ShptoxmlPrintInfo(sxData);

            ShptoxmlReadShapefile(sxData, SHPTOXML_READ_SHP_MODE_SCAN);
            ShptoxmlReadShapefile(sxData, SHPTOXML_READ_SHP_MODE_WRITE);

            ShptoxmlPrintNumFeaturesOutput(sxData);
            cout << endl;

            ShptoxmlCloseShapefile(sxData);
        }
    }//for//

    EsriShpVerify(numShps > 0, "SHAPEFILE-PATH must be specified");

    numFiles = numShps;
    filePaths = (char**) MEM_malloc(sizeof(char*) * numFiles);

    int i;
    BOOL wasFound;
    for (i = 0; i < numFiles; i++)
    {
        filePaths[i] = (char*) MEM_malloc(sizeof(char) * 256);

        BOOL fallBackToGlobal = FALSE;
        if (i == 0) { fallBackToGlobal = TRUE; }

        IO_ReadStringInstance(
            ANY_NODEID,
            ANY_ADDRESS,
            nodeInput,
            "SHAPEFILE-PATH",
            i,
            fallBackToGlobal,
            &wasFound,
            filePaths[i]);

        ShptoxmlSetFileSuffix(filePaths[i], ".xml");

        assert(wasFound);
    }//for//
}

bool
EsriShpShpReadParameters(
    TerrainData* terrainData,
    NodeInput* nodeInput,
    ShptoxmlData& sxData,
    int instanceId)
{
    BOOL parameterFound;
    char buf[MAX_STRING_LENGTH];

    BOOL fallBackToGlobal = FALSE;
    if (instanceId == 0) { fallBackToGlobal = TRUE; }

    IO_ReadStringInstance(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "SHAPEFILE-PATH",
        instanceId,
        fallBackToGlobal,
        &parameterFound,
        buf);

    if (!parameterFound)
    {
        return false;
    }

    EsriShpVerify(
        strlen(buf) < sizeof(sxData.m_shpPath),
        "SHAPEFILE-PATH too long");

    strcpy(sxData.m_shpPath, buf);

    strcpy(sxData.m_xmlPath, sxData.m_shpPath);
    ShptoxmlSetFileSuffix(sxData.m_xmlPath, ".xml");

    IO_ReadStringInstance(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "SHAPEFILE-DEFAULT-SHAPE-TYPE",
        instanceId,
        TRUE,
        &parameterFound,
        buf);

    if (parameterFound)
    {
        if (!strcmp(buf, "BUILDING")) { } // default
        else
        if (!strcmp(buf, "FOLIAGE"))
        {
            sxData.m_featureType = SHPTOXML_FOLIAGE;
        }
        else
        {
            EsriShpReportError("SHAPEFILE-DEFAULT-SHAPE-TYPE invalid");
        }
    }

    IO_ReadStringInstance(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "SHAPEFILE-DEFAULT-MSMT-UNIT",
        instanceId,
        TRUE,
        &parameterFound,
        buf);

    if (parameterFound)
    {
        if (!strcmp(buf, "FEET")) { } // default
        else
        if (!strcmp(buf, "METERS"))
        {
            sxData.m_shpUnits = SHPTOXML_METERS;
        }
        else
        {
            EsriShpReportError("SHAPEFILE-DEFAULT-MSMT-UNIT invalid");
        }
    }

    const char* heightParameterName = NULL;
    if (sxData.m_featureType == SHPTOXML_BUILDING)
    { heightParameterName = "SHAPEFILE-DEFAULT-BLDG-HEIGHT"; }
    else if (sxData.m_featureType == SHPTOXML_FOLIAGE)
    { heightParameterName = "SHAPEFILE-DEFAULT-FOLIAGE-HEIGHT"; }
    else
    { assert(0); }

    int int_defaultHeight;
    IO_ReadIntInstance(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        heightParameterName,
        instanceId,
        TRUE,
        &parameterFound,
        &int_defaultHeight);

    if (parameterFound)
    {
        EsriShpVerify(int_defaultHeight >= 0,
                       "SHAPEFILE-DEFAULT-{BLDG|FOLIAGE}-HEIGHT invalid");

        sxData.m_defaultHeight = (unsigned) int_defaultHeight;
    }

    if (sxData.m_featureType == SHPTOXML_FOLIAGE)
    {
        IO_ReadDoubleInstance(
            ANY_NODEID,
            ANY_ADDRESS,
            nodeInput,
            "SHAPEFILE-DEFAULT-FOLIAGE-DENSITY",
            instanceId,
            TRUE,
            &parameterFound,
            &sxData.m_defaultDensity);

        if (parameterFound)
        {
            EsriShpVerify(sxData.m_defaultDensity >= 0 && sxData.m_defaultDensity < 1.0,
                           "SHAPEFILE-DEFAULT-FOLIAGE-DENSITY invalid");
        }
    }

    IO_ReadStringInstance(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "SHAPEFILE-DBF-FILE-MSMT-UNIT",
        instanceId,
        TRUE,
        &parameterFound,
        buf);

    if (parameterFound)
    {
        if (!strcmp(buf, "FEET")) { } // default
        else
        if (!strcmp(buf, "METERS"))
        {
            sxData.m_dbfUnits = SHPTOXML_METERS;
        }
        else
        {
            EsriShpReportError("SHAPEFILE-DBF-FILE-MSMT-UNIT invalid");
        }
    }

    const char* dbfHeightParameterName = NULL;
    if (sxData.m_featureType == SHPTOXML_BUILDING)
    { dbfHeightParameterName = "SHAPEFILE-DBF-BLDG-HEIGHT-TAG-NAME"; }
    else if (sxData.m_featureType == SHPTOXML_FOLIAGE)
    { dbfHeightParameterName = "SHAPEFILE-DBF-FOLIAGE-HEIGHT-TAG-NAME"; }
    else
    { assert(0); }

    IO_ReadStringInstance(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        dbfHeightParameterName,
        instanceId,
        TRUE,
        &parameterFound,
        buf);

    if (parameterFound)
    {
        EsriShpVerify(
            strlen(buf) < sizeof(sxData.m_dbfHeightFieldName),
            "SHAPEFILE-DBF-{BLDG|FOLIAGE}-HEIGHT-TAG-NAME too long");

        strcpy(sxData.m_dbfHeightFieldName, buf);
    }

    IO_ReadStringInstance(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        dbfHeightParameterName,
        instanceId,
        TRUE,
        &parameterFound,
        buf);

    if (parameterFound)
    {
        EsriShpVerify(
            strlen(buf) < sizeof(sxData.m_dbfHeightFieldName),
            "SHAPEFILE-DBF-FOLIAGE-DENSITY-TAG-NAME too long");

        strcpy(sxData.m_dbfHeightFieldName, buf);
    }

    sxData.m_fileId = instanceId;

    return true;
}
