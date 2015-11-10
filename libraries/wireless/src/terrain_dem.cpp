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
#include "terrain_dem.h"
#include "qualnet_error.h"
#include "main.h"

#define ARC_SECONDS 3600.0

//
// DEM defines columns and rows in the following way:
//
//  NW         NE
//   ^^^^^^^^^^^
//   |||||||||||
//  R|||||||||||
//  o|||||||||||
//  w|||||||||||
//  s|||||||||||
//   |||||||||||
//  SW.........SE
//     Columns

#define DEM_INTEGER 6
#define DEM_DOUBLE  24


static
double ReadFloatingPointWithD(FILE *fp) {
    char tempString[1024];
    char *positionD;
    double returnValue;

    if (fscanf(fp, "%s", tempString) != 1)
    {
        ERROR_ReportError("TERRAIN-DEM: Unable to read floating"
                         " point value from DEM file \n");
    }

    positionD = strchr(tempString, 'D');

    if (positionD != NULL) {
        *positionD = 'E';
    }

    returnValue = (double)atof(tempString);
    return returnValue;
}

static
double ReadFloatingPointWithSize(FILE *fp, unsigned int size) {
    double  returnValue = 0.0;
    char*   tempString = (char*)MEM_malloc(size + 1);
    memset(tempString, 0, size + 1);

    if (fread(tempString, sizeof(char), size, fp) != (unsigned int)size)
    {
       ERROR_ReportWarning("Unable to read from file \n");
    }
    else
    {
        returnValue = (double)atof(tempString);
    }

    MEM_free(tempString);
    return returnValue;
}


static double ReadFloatingPoint(FILE * fp,int bytestoread){

    char tempString[1024];
    char *positionD;
    double returnValue;

    if (fgets(tempString, bytestoread, fp) == NULL)
    {
        ERROR_ReportError("TERRAIN-DEM: Unable to read floating"
                         " point value from DEM file \n");
    }

    positionD = strchr(tempString, 'D');

    if (positionD != NULL) {
        *positionD = 'E';
    }

    returnValue = (double)atof(tempString);
    return returnValue;

}



static int ReadInteger(FILE * fp, int bytestoread, int *value){

    char tempString[12];
    int position;

    if (fgets(tempString, bytestoread, fp) == NULL)
    {
        return -1;
    }

    position = (int)strcspn(tempString,"1234567890");

    if (position == (int)strlen(tempString)){
        return -1;
    }
    else{
        *value = atoi(tempString);
    }
    return 1;
}


static
void ReadTypeARecord(FILE *fp, DemTypeARecordData *a) {
    double latitude[4] = {0.0, 0.0, 0.0, 0.0};
    double longitude[4] = {0.0, 0.0, 0.0, 0.0};
    char str[150];
    int i;

    //
    // Get DEM Name (144 characters)
    //
    if (fgets(a->quadrangleName, QUADRANGLE_NAME_LENGTH + 1, fp) == NULL)
    {
        ERROR_ReportError("TERRAIN-DEM: Unable to read Quadrangle"
                         " name from type A record \n");
    }

    assert(a->quadrangleName[QUADRANGLE_NAME_LENGTH] == 0);

    for (i = QUADRANGLE_NAME_LENGTH - 1; i > 0; i--) {
        if (a->quadrangleName[i] != ' ') {
            break;
        }
    }
    assert(i >= 0);

    a->quadrangleName[i + 1] = 0;

    printf("DEM Name: \"%s\"\n", a->quadrangleName);

    //
    // Read the following 4 codes
    //


    int b = ReadInteger(fp,7,&(a->levelCode));
    ERROR_Assert(b != -1, "Level Code not found");

    b = ReadInteger(fp,7,&(a->patternCode));
    ERROR_Assert(b != -1, "Pattern code not found");

    b = ReadInteger(fp,7,&(a->planimetricCode));
    ERROR_Assert(b != -1, "Planimetric code not found");

    b = ReadInteger(fp,7,&(a->zoneCode));
    ERROR_Assert(b != -1, "Zone code not found");


    //assert(a->levelCode == 3);
    ERROR_Assert(a->patternCode == 1,
        "Invalid value found for Pattern Code. It should be 1");
    ERROR_Assert(a->planimetricCode == 0,
        "Invalid Code found for Planimteric Code. It should be 0 ");
    ERROR_Assert(a->zoneCode == 0,
        "Invalid Code found for Zone Code. It should be 0");

    //
    // Read the following 15 floating point values
    //

    for (i = 0; i < 15 ; i++)
    {
        a->projection[i] = (float) ReadFloatingPoint(fp,25);
        if (a->projection[i] != (float)0.0){
          sprintf(str,"A non zero value has been read for a->projection "
          "[ %d ]. This value should be 0. The value read has been ignored."
                    ,i);
           ERROR_ReportWarning(str);
        }
    }

    //
    // Get units code (must be 3 == arc-second)
    //
     b = ReadInteger(fp,7,&(a->gridUnit));
    ERROR_Assert(b != -1, "Grid unit not found");
    ERROR_Assert(a->gridUnit == 3,
        "Invalid Code found for Grid Unit. It should be 3");

    //
    // Get another units code (must be 2 == meter)
    //
    b = ReadInteger(fp,7,&(a->elevationUnit));
    ERROR_Assert(b != -1, "Elevation unit not found");
    ERROR_Assert(a->elevationUnit == 2,
      "Invalid Code found for Elevation Unit. It should be 2");

    //
    // Get number of sides (must be 4)
    //

    b = ReadInteger(fp,7,&(a->numSides));
    ERROR_Assert(b != -1, "Numsides not found");
    ERROR_Assert(a->numSides == 4,
    "Invalid value found for Number of Sides in polygon. It should be 4");

    //
    // Get the coordinates of 4 corners
    //

    for (i = 0 ; i < 4 ;i++){
        longitude[i] = ReadFloatingPoint(fp,25);
        latitude[i]  = ReadFloatingPoint(fp,25);
    }

    assert(latitude[0] == latitude[3]);
    assert(latitude[1] == latitude[2]);
    assert(longitude[0] == longitude[1]);
    assert(longitude[2] == longitude[3]);

    printf("Bounding box: SW (%f, %f) NE (%f, %f)\n",
           latitude[0] / ARC_SECONDS, longitude[0] / ARC_SECONDS,
           latitude[2] / ARC_SECONDS, longitude[2] / ARC_SECONDS);

    // Store lat/lon in familiar coordinates for easy matching.
    a->southWestCorner.latlonalt.latitude = latitude[0] / ARC_SECONDS;
    a->southWestCorner.latlonalt.longitude = longitude[0] / ARC_SECONDS;
    a->northEastCorner.latlonalt.latitude = latitude[2] / ARC_SECONDS;
    a->northEastCorner.latlonalt.longitude = longitude[2] / ARC_SECONDS;

    //
    // Get minimum and maximum elevations
    //
    a->minElevation = (float)ReadFloatingPoint(fp,25);
    a->maxElevation = (float)ReadFloatingPoint(fp,25);

    //
    // Read angle
    //
    a->angle = (float)ReadFloatingPoint(fp,25);
    ERROR_Assert(a->angle == 0.0,
    "Invalid value found for Angle. It should be 0");

    //
    // Read accuracy
    //

    b = ReadInteger(fp,7,&(a->accuracy));
    ERROR_Assert(b != -1, "Accuracy not found");
    ERROR_Assert(a->accuracy < 2,
    "Invalid Code found for accuracy. Accuraccy should be either 0 or 1");

    //
    // Read spatial resolution
    //

    for (i = 0;i < 3; i++){
        a->resolution[i] = (float)ReadFloatingPoint(fp,13);
    }


    //
    // Read rows and columns
    //

    b = ReadInteger(fp,7,&(a->numRows));
    ERROR_Assert(b != -1, "Num Rows not found");
    ERROR_Assert(a->numRows == 1," Invalid value found for Number of Rows. It should be 1");

    b = ReadInteger(fp,7,&(a->numColumns));
    ERROR_Assert(b != -1, "Number of Columns not found");


    //
    // Allocate Type B Record array
    //
    a->b = (DemTypeBRecordData *)
        MEM_malloc(a->numRows * a->numColumns * sizeof(DemTypeBRecordData));
    assert(a->b != NULL);

    //
    // Skipping a blank space (sometimes not) before Type B Record
    // It should be 165 bytes
    //
#define DEM_SKIP_BLANK_AFTER_TYPE_A_RECORD 165
    {
        char blank[DEM_SKIP_BLANK_AFTER_TYPE_A_RECORD + 1];

        if (fread(blank, sizeof(char), DEM_SKIP_BLANK_AFTER_TYPE_A_RECORD, fp)
            != DEM_SKIP_BLANK_AFTER_TYPE_A_RECORD)
        {
            ERROR_ReportError("TERRAIN-DEM: Unable to skip blank space"
                              " before type B record \n");
        }
    }
}


static
void ReadTypeBRecord(FILE *fp, int columnIndex, DemTypeBRecordData *b, float resolution) {
    int i;

    //
    // Get the indices and assert
    //


    if (fscanf(fp, "%d %d", &(b->rowIndex), &(b->columnIndex)) != 2)
    {
        ERROR_ReportError("TERRAIN-DEM: Unable to read row"
                         " index and column index for type B record\n");
    }

    assert(b->rowIndex == 1);
    assert(b->columnIndex == columnIndex);

    //
    // Get the number of rows and allocate memory for elevation data
    //
    if (fscanf(fp, "%d %d", &(b->numRows), &(b->numColumns)) != 2)
    {
        ERROR_ReportError("TERRAIN-DEM: Unable to read number of rows"
                         " and number of columns of type B record \n");
    }

    b->elevationData = (short *)MEM_malloc(b->numRows * sizeof(short));
    assert(b->numColumns == 1);

    //
    // Get the coordinates
    //
    b->firstPoint.latlonalt.longitude = ReadFloatingPointWithSize(fp,DEM_DOUBLE);
    b->firstPoint.latlonalt.latitude  = ReadFloatingPointWithSize(fp,DEM_DOUBLE);

    //
    // Get the reference elevation
    //
    b->referenceElevation = (float)ReadFloatingPointWithSize(fp, DEM_DOUBLE);
    assert(b->referenceElevation == 0.0);

    //
    // Get minimum and maximum elevations
    //
    b->minElevation = (float)ReadFloatingPointWithSize(fp, DEM_DOUBLE);
    b->maxElevation = (float)ReadFloatingPointWithSize(fp, DEM_DOUBLE);

    //
    // Read elevation data
    //
    for (i = 0; i < b->numRows; i++)
    {
        if (fscanf(fp, "%hd", &(b->elevationData[i])) != 1)
        {
           ERROR_ReportError("TERRAIN-DEM: Unable to read elevation data"
                            " for type B record\n");
        }
    }
}


static
void ReadTypeCRecord(FILE *fp, DemTypeCRecordData *c) {
    int i;

    for (i = 0; i < 10; i++)
    {
        if (fscanf(fp, "%d", &(c->value[i])) != 1)
        {
            ERROR_ReportError("TERRAIN-DEM: Unable to read type C record \n");
       }
    }
}


   DemTerrainData::~DemTerrainData()
   {
       unsigned int i;
       // free data
       for (i = 0; i < m_numDemFiles; i++) {
           DemTypeARecordData* a = m_records[i];
           MEM_free(a->b->elevationData);
           MEM_free(a->b);
           MEM_free(a);
   }

}

void DemTerrainData::initialize(NodeInput* nodeInput) {
    int i;
    char terrainFilename[MAX_STRING_LENGTH];
    BOOL wasFound;
    FILE *fp;
    int fileIndex = 0;

    while (TRUE) {
        DemTypeARecordData* a;

        IO_ReadStringInstance(
            ANY_NODEID,
            ANY_ADDRESS,
            nodeInput,
            "DEM-FILENAME",
            fileIndex,
            (fileIndex == 0),
            &wasFound,
            terrainFilename);

        if (wasFound != TRUE) {
            break;
        }

        assert(fileIndex < MAX_NUM_DEM_FILES);

        fp = fopen(terrainFilename, "r");

        if (fp == NULL) {
            char errorMessage[MAX_STRING_LENGTH];

            sprintf(errorMessage, "Cannot open DEM-FILENAME: %s",
                    terrainFilename);
            ERROR_ReportError(errorMessage);
        }

        a = (DemTypeARecordData*)MEM_malloc(sizeof(DemTypeARecordData));

        m_records[fileIndex] = a;

        //
        // Read Type A Record
        //
        ReadTypeARecord(fp, a);

        //
        // Read Type B Records
        //
        for (i = 0; i < a->numColumns; i++) {
            ReadTypeBRecord(fp, i + 1, &(a->b[i]), a->resolution[2]);
        }

        //
        // Read Type C Record
        //
        if (a->accuracy == 1) {
            ReadTypeCRecord(fp, &(a->c));
        }

        fclose(fp);

        fileIndex++;
    }

    if (fileIndex == 0) {
        ERROR_ReportError("Cannot find DEM-FILENAME in the configuration file");
    }

    for (i = fileIndex; i < MAX_NUM_DEM_FILES; i++) {
        m_records[i] = NULL;
    }

    m_numDemFiles = fileIndex;

    return;
}

double DemTerrainData::getElevationAt(const Coordinates* point)
{
    short northWestElevation;
    short northEastElevation;
    short southWestElevation;
    short southEastElevation;

    CoordinateType elevation;

    // find the file containing this coordinate.
    int matchingFile = findMatchingFile(point);

    if (matchingFile == DEM_NO_MATCH_FOUND) {
        if (m_terrainData->checkBoundaries()) {
                char errorMessage[MAX_STRING_LENGTH];

                sprintf(errorMessage,
                    "Point (%f, %f) is not in the terrain database\n",
                    point->common.c1, point->common.c2);
                ERROR_ReportError(errorMessage);
            }
        else {
            return 0.0;
        }
    }

    {
        const DemTypeARecordData* const a = m_records[matchingFile];

    const double normalizedLongitude =
            ARC_SECONDS * (point->latlonalt.longitude -
         a->southWestCorner.latlonalt.longitude) / a->resolution[0];
    const double normalizedLatitude =
            ARC_SECONDS * (point->latlonalt.latitude -
         a->southWestCorner.latlonalt.latitude) / a->resolution[1];

    const int northWestLatitude = (int)ceil(normalizedLatitude);
    const int northWestLongitude = (int)floor(normalizedLongitude);

    const double dLatitude = northWestLatitude - normalizedLatitude;
    const double dLongitude = normalizedLongitude - northWestLongitude;

    assert(a != NULL);
    assert(dLatitude >= 0.0 && dLatitude < 1.0);
    assert(dLongitude >= 0.0 && dLongitude < 1.0);
    assert(northWestLongitude >= 0 &&
           northWestLongitude < a->numColumns);
    assert(northWestLatitude >= 0 &&
           northWestLatitude < a->b[northWestLongitude].numRows);

    northWestElevation =
        a->b[northWestLongitude].elevationData[northWestLatitude];

    if (northWestLongitude < a->numColumns - 1 && northWestLatitude > 0) {
        southEastElevation =
            a->b[northWestLongitude + 1].elevationData[northWestLatitude - 1];
    }
    else {
        assert(dLatitude == 0.0 || dLongitude == 0.0);
        southEastElevation = 0;
    }

    if (dLatitude > dLongitude) {
        southWestElevation =
            a->b[northWestLongitude].elevationData[northWestLatitude - 1];

            elevation = northWestElevation +
            (southEastElevation - southWestElevation) * dLongitude +
            (southWestElevation - northWestElevation) * dLatitude;
    }
    else {
        northEastElevation =
            a->b[northWestLongitude + 1].elevationData[northWestLatitude];

            elevation = northWestElevation +
            (northEastElevation - northWestElevation) * dLongitude +
            (southEastElevation - northEastElevation) * dLatitude;
    }
    }
    return elevation;
}

void DemTerrainData::getElevationBoundaries(int index, Coordinates* sw, Coordinates* ne)
{
    ERROR_Assert(index >= 0 && index < m_numDemFiles, "Invalid index");

    *sw = m_records[index]->southWestCorner;
    *ne = m_records[index]->northEastCorner;
}

void DemTerrainData::getHighestAndLowestElevation(
    const Coordinates* sw,
    const Coordinates* ne,
    double* highest,
    double* lowest)
{
    // rather than searching all the entries over multiple tiles, we're
    // cheating and pulling the max/min from the whole tile.  This is lazy
    // but this function is really used for subdividing the terrain into
    // regions and that's mostly for urban terrain.

    int swIndex = findMatchingFile(sw);
    if (swIndex == DEM_NO_MATCH_FOUND) {
        // this is really an error
        *highest = 0.0;
        *lowest  = 0.0;
    }
    *highest = m_records[swIndex]->maxElevation;
    *lowest  = m_records[swIndex]->minElevation;
    int neIndex = findMatchingFile(ne);
    if (neIndex == DEM_NO_MATCH_FOUND) {
        // this is really an error
    return;
}
    else if (swIndex != neIndex) {
        // this probably means that the specified coordinates overlap at least
        // two tiles, but possibly four or more.  Let's cheat.
        *highest = MAX(*highest, m_records[neIndex]->maxElevation);
        *lowest  = MIN(*lowest, m_records[swIndex]->minElevation);
    }
}

UInt32 DemTerrainData::findMatchingFile(const Coordinates* point) {

    // have to be careful with m_mostRecentFile to ensure thread safety

    UInt32 lastMatch = m_mostRecentFile; // make a local copy
    UInt32 index = 0;

    // check for corrupted value
    if (lastMatch >= m_numDemFiles) {
        lastMatch = 0;
    }

    if (COORD_PointWithinRange(m_terrainData->getCoordinateSystem(),
                               &(m_records[lastMatch]->southWestCorner),
                               &(m_records[lastMatch]->northEastCorner),
                               point)) {
        return lastMatch;
}

    for (; index < m_numDemFiles; index++) {
        if ((index != lastMatch) &&
                COORD_PointWithinRange(m_terrainData->getCoordinateSystem(),
                                       &(m_records[index]->southWestCorner),
                                       &(m_records[index]->northEastCorner),
                                       point)) {
            m_mostRecentFile = index;
            return index;
    }
    }

    return DEM_NO_MATCH_FOUND;
}
