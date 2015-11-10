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
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <climits>

// GCC 4.3+ requires CSTRING in this library, gcc 4.2- does not
#define GCC_VERSION (__GNUC__ * 10000 \
                     + __GNUC_MINOR__ * 100 \
                     + __GNUC_PATCHLEVEL__)
/* Test for GCC > 4.2.0 */
#if GCC_VERSION > 40200
#include <cstring>
#endif



#include "shptoxml_shared.h"

void
ShptoxmlInitData(ShptoxmlData& sxData)
{
    // memset() memory to 0 and assign (hard-coded) default values.
    // (These defaults are changed later in the program via command-line
    // arguments, .config file settings, etc.)

    memset(&sxData, 0, sizeof(sxData));

    sxData.m_featureType = SHPTOXML_BUILDING;
    sxData.m_shpUnits = SHPTOXML_FEET;
    sxData.m_defaultHeight = 35;
    sxData.m_defaultHeightInMeters
        = (unsigned) ((double) sxData.m_defaultHeight
                      * SHPTOXML_FEET_TO_METERS);
    sxData.m_defaultDensity = 0.15;

    sxData.m_maxUserFeatures = UINT_MAX;

    sxData.m_minUserLat = -90.0;
    sxData.m_maxUserLat = 90.0;
    sxData.m_minUserLon = -180.0;
    sxData.m_maxUserLon = 180.0;

    sxData.m_minLat     = 90.0;
    sxData.m_maxLat     = -90.0;
    sxData.m_minLon     = 180.0;
    sxData.m_maxLon     = -180.0;

    sxData.m_dbfUnits = SHPTOXML_FEET;
    strcpy(sxData.m_dbfHeightFieldName, "LV");
    strcpy(sxData.m_dbfDensityFieldName, "DENSITY");

    sxData.m_fileId = 0;
}

void
ShptoxmlOpenShapefile(ShptoxmlData& sxData)
{
    sxData.m_hShp = SHPOpen(sxData.m_shpPath, "rb");

    ShptoxmlVerify(
        sxData.m_hShp != NULL,
        "Unable to open shapefile (.shp and .shx)");

    sxData.m_hDbf = DBFOpen(sxData.m_shpPath, "rb");
}

void
ShptoxmlReadShpHeader(ShptoxmlData& sxData)
{
    int int_numShapes;

    SHPGetInfo(
        sxData.m_hShp,
        &int_numShapes,
        &sxData.m_shapeType,
        sxData.m_minExtents,
        sxData.m_maxExtents);

    ShptoxmlVerify(int_numShapes >= 0, "Negative number of shapes");

    sxData.m_numShapes = (unsigned) int_numShapes;
}

void
ShptoxmlReadDbfFieldNames(ShptoxmlData& sxData)
{
    assert(sxData.m_hDbf != NULL);

    // Height

    sxData.m_dbfHeightFieldIndex
        = DBFGetFieldIndex(sxData.m_hDbf, sxData.m_dbfHeightFieldName);

    if (sxData.m_dbfHeightFieldIndex >= 0)
    {
        sxData.m_dbfFoundHeightField = true;
    }

    // Foliage density

    if (sxData.m_featureType == SHPTOXML_FOLIAGE)
    {
        sxData.m_dbfDensityFieldIndex
            = DBFGetFieldIndex(sxData.m_hDbf, sxData.m_dbfDensityFieldName);

        if (sxData.m_dbfDensityFieldIndex >= 0)
        {
            sxData.m_dbfFoundDensityField = true;
        }
    }
}

void
ShptoxmlCheckShpDbfRecordCountConsistency(ShptoxmlData& sxData)
{
    assert(sxData.m_hDbf != NULL);

    ShptoxmlVerify(
        (unsigned) DBFGetRecordCount(sxData.m_hDbf) == sxData.m_numShapes,
        "Shape count doesn't match between .shp and .dbf files");
}

void
ShptoxmlPrintInfo(ShptoxmlData& sxData)
{
    cout << "Shapefile .shp file         = " << sxData.m_shpPath << endl;

    cout << "Shapefile feature type      = ";
    if (sxData.m_featureType == SHPTOXML_BUILDING)
    { cout << "Building" << endl; }
    else if (sxData.m_featureType == SHPTOXML_FOLIAGE)
    { cout << "Foliage" << endl; }
    else { assert(0); }

    cout << "Feature height (default)    = "
            << sxData.m_defaultHeightInMeters << " meter(s)" << endl;
    cout << "Non-.dbf file units         = ";

    if (sxData.m_shpUnits == SHPTOXML_FEET)
    { cout << "Feet" << endl; }
    else
    if (sxData.m_shpUnits == SHPTOXML_METERS)
    { cout << "Meters" << endl; }
    else
    { assert(0); }

    cout << ".dbf file found             = ";
    if (sxData.m_hDbf != NULL)
    { cout << "Yes" << endl; }
    else
    { cout << "No" << endl; }

    cout << ".dbf units                  = ";
    if (sxData.m_dbfUnits == SHPTOXML_FEET)
    { cout << "Feet" << endl; }
    else
    if (sxData.m_dbfUnits == SHPTOXML_METERS)
    { cout << "Meters" << endl; }
    else
    { assert(0); }

    cout << ".dbf height field name      = " << sxData.m_dbfHeightFieldName;

    if (sxData.m_dbfFoundHeightField)
    { cout << " (found)" << endl; }
    else
    { cout << " (not found)" << endl; }

    cout << ".dbf density field name     = " << sxData.m_dbfDensityFieldName;

    if (sxData.m_dbfFoundDensityField)
    { cout << " (found)" << endl; }
    else
    { cout << " (not found)" << endl; }

    if (sxData.m_featureType == SHPTOXML_FOLIAGE)
    { cout << "Foliage density             = " << sxData.m_defaultDensity << endl; }

    if (sxData.m_maxUserFeatures != UINT_MAX)
    {
        cout << "Max features                 = " << sxData.m_maxUserFeatures
                  << endl;
    }

    printf(
        "User boundaries             = (%.6f, %.6f)\n"
        "                              (%.6f, %.6f)\n",
        sxData.m_minUserLat,
        sxData.m_minUserLon,
        sxData.m_maxUserLat,
        sxData.m_maxUserLon);

    cout << "Number of shapes            = " << sxData.m_numShapes << endl;
}

void
ShptoxmlPrintShapes(ShptoxmlData& sxData)
{
    unsigned shapeIndex;
    for (shapeIndex = 0; shapeIndex < sxData.m_numShapes; shapeIndex++ )
    {
        SHPObject *shape = SHPReadObject(sxData.m_hShp, shapeIndex);

        // Filter for shapes which are completely within bounds.

        if (!(shape->dfYMin >= sxData.m_minUserLat
              && shape->dfXMin >= sxData.m_minUserLon
              && shape->dfYMax <= sxData.m_maxUserLat
              && shape->dfXMax <= sxData.m_maxUserLon))
        { continue; }

        printf(
            "\n"
            "Shape:%d (%s) nVertices=%d\n"
            "  Bounds:(%12.6f,%12.6f)\n"
            "      to (%12.6f,%12.6f)\n",
            shapeIndex, SHPTypeName(shape->nSHPType), shape->nVertices,
            shape->dfXMin, shape->dfYMin,
            shape->dfXMax, shape->dfYMax);

        int vertexIndex;
        for (vertexIndex = 0; vertexIndex < shape->nVertices; vertexIndex++)
        {
            printf("      %2d (%12.6f,%12.6f)\n",
                   vertexIndex,
                   shape->padfX[vertexIndex],
                   shape->padfY[vertexIndex]);
        }

        SHPDestroyObject(shape);
        shape = NULL;
    }
}

void
ShptoxmlCreateShapefile(ShptoxmlData& sxData)
{
    assert(sxData.m_writeNewShp);
    assert(sxData.m_newShpPath[0] != 0);
    assert(sxData.m_hNewShp == NULL);

    sxData.m_hNewShp = SHPCreate(sxData.m_newShpPath, SHPT_POLYGON);

    ShptoxmlVerify(
        sxData.m_hNewShp != NULL,
        "Unable to write new shapefile");
}

void
ShptoxmlReadShapefile(ShptoxmlData& sxData, ShptoxmlReadShpMode mode)
{
    FILE* fpXml = NULL;

    if (mode == SHPTOXML_READ_SHP_MODE_WRITE)
    {
        // Open new file for writing.

        fpXml = fopen(sxData.m_xmlPath, "w");

        ShptoxmlVerify(fpXml != NULL, "Can't write .xml file");

        // Write xml, site, region tags.

        fprintf(fpXml, "<?xml version=\"1.0\"?>\n");
        fprintf(
            fpXml,
            "<Site Name=\"Site0\" id=\"Site0\""
            " ReferencePoint=\"0.0 0.0 0.0\" CoordinateType=\"geodetic\""
            " part=\"1\" totalParts=\"01\""
            " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
            " xsi:noNamespaceSchemaLocation=\"file:qualnet-road.xsd\">\n");

        fprintf(
            fpXml,
            "<Region id=\"Region0\" CoordinateType=\"geodetic\">\n");

        fprintf(
            fpXml,
            "<position>%.6f %.6f 0.0</position>\n"
              "<position>%.6f %.6f 0.0</position>\n"
              "<position>%.6f %.6f 0.0</position>\n"
              "<position>%.6f %.6f 0.0</position>\n",
            sxData.m_minLon, sxData.m_minLat,
            sxData.m_minLon, sxData.m_maxLat,
            sxData.m_maxLon, sxData.m_maxLat,
            sxData.m_maxLon, sxData.m_minLat);

        fprintf(fpXml, "</Region>\n");
    }//if//

    // Write feature tags.

    const char* featureType = NULL;
    if (sxData.m_featureType == SHPTOXML_BUILDING)
    { featureType = "Building"; }
    else if (sxData.m_featureType == SHPTOXML_FOLIAGE)
    { featureType = "Foliage"; }
    else { assert(0); }

    unsigned shapeIndex;
    unsigned featureIndex = 0;
    SHPObject* shape = NULL;

    for (shapeIndex = 0; shapeIndex < sxData.m_numShapes; shapeIndex++ )
    {
        // Clean up previous SHPReadObject() call.

        if (shape != NULL)
        {
            SHPDestroyObject(shape);
            shape = NULL;
        }

        // Exit early if we've already reached max feature count.

        if (featureIndex >= sxData.m_maxUserFeatures) { break; }

        assert(shape == NULL);
        shape = SHPReadObject(sxData.m_hShp, shapeIndex);

        // Filter for shapes which are completely within bounds.

        if (!(shape->dfYMin >= sxData.m_minUserLat
              && shape->dfXMin >= sxData.m_minUserLon
              && shape->dfYMax <= sxData.m_maxUserLat
              && shape->dfXMax <= sxData.m_maxUserLon))
        { continue; }

        // Filter any "point" or "line" footprints (but allow triangles and
        // above).

        if (shape->nVertices < 3)
        { continue; }

        // Get height of feature from .dbf file; if not valid, use default.

        unsigned height = sxData.m_defaultHeightInMeters;

        if (sxData.m_dbfFoundHeightField)
        {
            height =
                DBFReadIntegerAttribute(
                    sxData.m_hDbf,
                    shapeIndex,
                    sxData.m_dbfHeightFieldIndex);

            if (height == 0)
            {
                // Use default height when value is returned as 0.

                height = sxData.m_defaultHeightInMeters;
            }
            else
            if (sxData.m_dbfUnits == SHPTOXML_FEET)
            {
                height = (unsigned)
                             ((double) height * SHPTOXML_FEET_TO_METERS);

                // Enforce 1-meter minimum height when rounding a non-zero
                // feet value to 0 meters.

                if (height == 0) { height = 1; }
            }
        }

        double density = sxData.m_defaultDensity;

        if (sxData.m_featureType == SHPTOXML_FOLIAGE
            && sxData.m_dbfFoundDensityField)
        {
            density =
                DBFReadDoubleAttribute(
                    sxData.m_hDbf,
                    shapeIndex,
                    sxData.m_dbfDensityFieldIndex);

            if (density == 0.0)
            {
                // Use default density when value is returned as 0.

                density = sxData.m_defaultDensity;
            }
        }

        if (mode == SHPTOXML_READ_SHP_MODE_WRITE)
        {
            fprintf(
                fpXml,
                "<%s id=\"%u%c%03d\" Name=\"%u%c%03d\"",
                featureType,
                sxData.m_fileId,
                featureType[0],
                featureIndex,
                sxData.m_fileId,
                featureType[0],
                featureIndex);

            // Write foliage attributes if we need to.

            if (sxData.m_featureType == SHPTOXML_FOLIAGE)
            {
                fprintf(fpXml, " density=\"%.2f\"", density);
            }

            fprintf(fpXml, ">\n");

            // Build faces (vertical).

            int vertexIndex;
            unsigned faceIndex;
            for (vertexIndex = 0, faceIndex = 0;
                 vertexIndex < (shape->nVertices - 1);
                 vertexIndex++, faceIndex++)
            {
                fprintf(
                    fpXml,
                    "<face Name=\"F%d\" id=\"F%d\">\n",
                    faceIndex,
                    faceIndex);

                fprintf(
                    fpXml,
                    "<position>%.6f %.6f %d</position>\n",
                    shape->padfX[vertexIndex],
                    shape->padfY[vertexIndex],
                    0);

                fprintf(
                    fpXml,
                    "<position>%.6f %.6f %d</position>\n",
                    shape->padfX[vertexIndex + 1],
                    shape->padfY[vertexIndex + 1],
                    0);

                fprintf(
                    fpXml,
                    "<position>%.6f %.6f %d</position>\n",
                    shape->padfX[vertexIndex + 1],
                    shape->padfY[vertexIndex + 1],
                    height);

                fprintf(
                    fpXml,
                    "<position>%.6f %.6f %d</position>\n",
                    shape->padfX[vertexIndex],
                    shape->padfY[vertexIndex],
                    height);

                fprintf(fpXml, "</face>\n");
            }//for//

            // Build ceiling (horizontal).

            fprintf(
                fpXml,
                "<face Name=\"F%d\" id=\"F%d\">\n",
                faceIndex,
                faceIndex);
        }//if//

        int vertexIndex;
        for (vertexIndex = 0;
             vertexIndex < (shape->nVertices - 1);
             vertexIndex++)
        {
            if (mode == SHPTOXML_READ_SHP_MODE_SCAN)
            {
                if (shape->padfX[vertexIndex] < sxData.m_minLon)
                { sxData.m_minLon = shape->padfX[vertexIndex]; }
                if (shape->padfX[vertexIndex] > sxData.m_maxLon)
                { sxData.m_maxLon = shape->padfX[vertexIndex]; }

                if (shape->padfY[vertexIndex] < sxData.m_minLat)
                { sxData.m_minLat = shape->padfY[vertexIndex]; }
                if (shape->padfY[vertexIndex] > sxData.m_maxLat)
                { sxData.m_maxLat = shape->padfY[vertexIndex]; }
            }
            else
            if (mode == SHPTOXML_READ_SHP_MODE_WRITE)
            {
                fprintf(
                    fpXml,
                    "<position>%.6f %.6f %d</position>\n",
                    shape->padfX[vertexIndex],
                    shape->padfY[vertexIndex],
                    height);
            }
        }//for//

        if (mode == SHPTOXML_READ_SHP_MODE_WRITE)
        {
            fprintf(fpXml, "</face>\n");

            fprintf(fpXml, "</%s>\n", featureType);

            if (sxData.m_writeNewShp)
            {
                SHPWriteObject(
                    sxData.m_hNewShp,
                    -1,        // new shape
                    shape);
            }
        }

        featureIndex++;
    }//for//

    // Clean up leftover SHPReadObject() call.

    if (shape != NULL)
    {
        SHPDestroyObject(shape);
        shape = NULL;
    }

    if (mode == SHPTOXML_READ_SHP_MODE_SCAN)
    {
        sxData.m_numFeatures = featureIndex;
    }
    else
    if (mode == SHPTOXML_READ_SHP_MODE_WRITE)
    {
        ShptoxmlVerify(
            featureIndex == sxData.m_numFeatures,
            "Wrong number of features");
    }

    if (mode == SHPTOXML_READ_SHP_MODE_WRITE)
    {
        // Close out site tag.

        fprintf(fpXml, "</Site>\n");

        fclose(fpXml);
    }
}

void
ShptoxmlPrintBoundaries(ShptoxmlData& sxData)
{
    printf(
        "TERRAIN-SOUTH-WEST-CORNER  (%.6f, %.6f)\n",
        sxData.m_minLat, sxData.m_minLon);

    printf(
        "TERRAIN-NORTH-EAST-CORNER  (%.6f, %.6f)\n",
        sxData.m_maxLat, sxData.m_maxLon);
}

void
ShptoxmlPrintNumFeaturesOutput(ShptoxmlData& sxData)
{
    cout << "Features output             = " << sxData.m_numFeatures << endl;
}

void
ShptoxmlCloseShapefile(ShptoxmlData& sxData)
{
    if (sxData.m_hShp != NULL)
    {
        SHPClose(sxData.m_hShp);
        sxData.m_hShp = NULL;
    }

    if (sxData.m_hDbf != NULL)
    {
        DBFClose(sxData.m_hDbf);
        sxData.m_hDbf = NULL;
    }

    if (sxData.m_hNewShp != NULL)
    {
        SHPClose(sxData.m_hNewShp);
        sxData.m_hNewShp = NULL;
    }
}
