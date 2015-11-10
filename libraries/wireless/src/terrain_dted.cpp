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
#include "terrain_dted.h"

#define DEBUG 0

#define ARC_SECONDS 3600.0

#define DTED_UHL_LENGTH 80
#define DTED_DSI_RECORD_LENGTH 648
#define DTED_ACC_RECORD_LENGTH 2700

//
// DTED defines columns and rows in the following way.
//
//  NW         NE
//   ^^^^^^^^^^^
//   |||||||||||
//  C|||||||||||
//  o|||||||||||
//  l|||||||||||
//  u|||||||||||
//  m|||||||||||
//  n|||||||||||
//  s|||||||||||
//   |||||||||||
//  SW.........SE
//       Rows


static void DtedReadUserHeaderLabel(FILE *fp, DtedRecordData *a) {
    char charStream[DTED_UHL_LENGTH];
    char tempString[1024];
    int numReturned;
    int pointer = 0;

    numReturned =
        (int)fread(charStream, sizeof(char), DTED_UHL_LENGTH, fp);

    assert(numReturned == DTED_UHL_LENGTH);

    if (strncmp(charStream, "UHL", 3) != 0) {
        assert(strncmp(charStream, "HDR", 3) == 0);

        numReturned =
            (int)fread(charStream, sizeof(char), DTED_UHL_LENGTH, fp);

        assert(numReturned == DTED_UHL_LENGTH);
        assert(strncmp(charStream, "UHL", 3) == 0);
    }

    pointer += 3;
    assert(charStream[pointer] == '1');
    pointer++;

     // Skip SW coordinates and read resolution.
     pointer += 16;
     memcpy(tempString, &(charStream[pointer]), 4);
    pointer += 4;
    tempString[4] = 0;

    // 21: latitude data interval: in tenths of seconds
    a->resolution[0] = (double)atoi(tempString) / 10.0;

    memcpy(tempString, &(charStream[pointer]), 4);
    pointer += 4;
    tempString[4] = 0;

    // 25: longitude data interval: in tenths of seconds
    a->resolution[1] = (double)atoi(tempString) / 10.0;

    // usually "  NA"
    a->resolution[2] = 1.0; // in meters

    pointer += (4 + 3 + 12); // skipping unnecessary fields

    // Number of longitude lines
    memcpy(tempString, &(charStream[pointer]), 4);
    pointer += 4;
    tempString[4] = 0;

    a->numRows = (int)atoi(tempString);

    // Number of latitude points
    memcpy(tempString, &(charStream[pointer]), 4);
    pointer += 4;
    tempString[4] = 0;

    a->numColumns = (int)atoi(tempString);

    // 120 + 1  for DTED Level 0, 30 arc-sec, ~1000m
    // 1200 + 1 for DTED Level 1,  3 arc-sec, ~100m
    // 3600 + 1 for DTED Level 2,  1 arc-sec, ~30m

    return;
}


//
// Read Data Set Identification (DSI) Record.
// SW and NE latitudes & longitudes are read to create a bounding rectangle
//
static
void DtedReadDataSetIdentificationRecord(FILE *fp, DtedRecordData *a) {
    char charStream[DTED_DSI_RECORD_LENGTH];
    char degreeString[4];
    char minutesString[3];
    char secondsString[3];
    int numReturned;
    int pointer = 0;
    double degrees = 0.0;
    double minutes = 0.0;
    double seconds = 0.0;
    int count = 1;
    numReturned =
        (int)fread(charStream, sizeof(char), DTED_DSI_RECORD_LENGTH, fp);

    assert(numReturned == DTED_DSI_RECORD_LENGTH);
    assert(strncmp(charStream, "DSI", 3) == 0);

    // Reach the position to read SW Latitude
    pointer += 204;

    while (count != 5)
    {
        memset(degreeString,0,sizeof(degreeString));
        if (count == 1 || count == 3 ){
            // Latitude calculations
            memcpy(degreeString, &(charStream[pointer]),2);
            pointer += 2;
        }
        else{
            // Longitude calculations
            memcpy(degreeString, &(charStream[pointer]),3);
            pointer += 3;
        }
        degrees = (double)atoi(degreeString);

        memset(minutesString,0,sizeof(minutesString));
        memcpy(minutesString, &(charStream[pointer]),2);
        pointer += 2;
        minutes = (double)atoi(minutesString);

        memset(secondsString,0,sizeof(secondsString));
        memcpy(secondsString, &(charStream[pointer]),2);
        pointer += 2;
        seconds = (double)atoi(secondsString);

        degrees += ((minutes / 60.0) + (seconds / 3600.0));
        switch (count){
            case 1: // SW Latitude
            {
                if (charStream[pointer] == 'N') {
                    // It's the north hemisphere
                    a->southWestCorner.latlonalt.latitude = degrees;
                }
                else {
                    // It's the south hemisphere
                    assert(charStream[pointer] == 'S');
                    a->southWestCorner.latlonalt.latitude = -degrees;
                }
                break;
            } //case 1
            case 2: // SW Longitude
            {
                if (charStream[pointer] == 'E') {
                    // It's the east hemisphere
                    a->southWestCorner.latlonalt.longitude = degrees;
                }
                else {
                    // It's the west hemisphere
                    assert(charStream[pointer] == 'W');
                    a->southWestCorner.latlonalt.longitude = -degrees;
                }
                pointer += 15; // Do not read NW coordinates
                break;
            } //case 2
            case 3: // NE Latitude
            {
                if (charStream[pointer] == 'N') {
                    // It's the north hemisphere
                    a->northEastCorner.latlonalt.latitude = degrees;
                }
                else {
                    // It's the south hemisphere
                    assert(charStream[pointer] == 'S');
                    a->northEastCorner.latlonalt.latitude = -degrees;
                }
               break;
            } //case 3
            case 4: // NE Longitude
            {
                if (charStream[pointer] == 'E') {
                    // It's the east hemisphere
                    a->northEastCorner.latlonalt.longitude = degrees;
                }
                else {
                    // It's the west hemisphere
                    assert(charStream[pointer] == 'W');
                    a->northEastCorner.latlonalt.longitude = -degrees;
                }
                break;
            } //case 4
        } //switch
       pointer++;
        count++; // increment the counter for reading next value
    }
}

//
// Read Accuracy Description (ACC) Record: currently not read
//
static
void DtedReadAccuracyDescriptionRecord(FILE *fp) {
    char byteStream[DTED_ACC_RECORD_LENGTH];
    int numBytes;
    numBytes =
        (int)fread(byteStream, sizeof(char), DTED_ACC_RECORD_LENGTH, fp);

    assert(numBytes == DTED_ACC_RECORD_LENGTH);
    assert(strncmp(byteStream, "ACC", 3) == 0);
}


//
// Read Elevation Data
//
static
void DtedReadElevationData(FILE *fp, DtedRecordData *a) {
    int dataSize = a->numColumns * 2 + 12;
    unsigned char* byteStream = (unsigned char*) MEM_malloc(dataSize * sizeof(unsigned char));
    int i;

    a->elevationData = (short**)MEM_malloc(a->numRows * sizeof(short*));
    assert(a->elevationData != NULL);

    for (i = 0; i < a->numRows; i++) {
        a->elevationData[i] =
            (short*)MEM_malloc(a->numColumns * sizeof(short));
        assert(a->elevationData[i] != NULL);
    }

    for (i = 0; i < a->numRows; i++) {
        int j;
        unsigned short elevation;
        // Setting all entries to Zero (Flat Surface)
        memset(byteStream,0,dataSize); // Reset

        int numBytes = (int)fread(byteStream, sizeof(char), dataSize, fp);

        for (j = 0; j < a->numColumns; j++) {
            //
            // 8 bytes before elevation data
            // (and 4 bytes after them)
            //
            int index = j + 4;

            elevation = byteStream[2 * index];
            elevation <<= 8;
            elevation = elevation + 
                     (unsigned short)byteStream[2 * index + 1];

            // max. range: +/- 32767
            // Null data value: -32767
            // elevation is unsigned, elevationData is signed

            if (elevation > 32767)
            {
                if (elevation == 65535) // null
                {
                    a->elevationData[i][j] = 0;
                }
                else    // negative
                {
                    elevation &= 0x7fff; // turn off 15th bit
                    a->elevationData[i][j] = (signed short)elevation * (-1);
                }
            }
            else
            {   // positive
                 a->elevationData[i][j] = elevation;
            }
            }
            }
        }

DtedTerrainData::~DtedTerrainData() {
    UInt32 i;
    int r;

    if (DEBUG) printf("DTED destructor called\n");
    // free data
    for (i = 0; i < m_numFiles; i++) {
        DtedRecordData* d = m_records[i];
        for (r = 0; r < d->numRows; r++) {
            MEM_free(d->elevationData[r]);
        }
        MEM_free(d->elevationData);
        MEM_free(d);
    }
}

void DtedTerrainData::initialize(NodeInput* nodeInput) {
    int i;
    char terrainFilename[MAX_STRING_LENGTH];
    BOOL wasFound;
    FILE *fp;
    int fileIndex = 0;

    while (TRUE) {
        DtedRecordData* a;

        IO_ReadStringInstance(
            ANY_NODEID,
            ANY_ADDRESS,
            nodeInput,
            "DTED-FILENAME",
            fileIndex,
            (fileIndex == 0),
            &wasFound,
            terrainFilename);

        if (wasFound != TRUE) {
            break;
        }

        assert(fileIndex < MAX_NUM_DTED_FILES);

        fp = fopen(terrainFilename, "rb");

        if (fp == NULL) {
            char errorMessage[MAX_STRING_LENGTH];

            sprintf(errorMessage, "Cannot open DTED-FILENAME: %s",
                    terrainFilename);
            ERROR_ReportError(errorMessage);
        }

        a = (DtedRecordData*)MEM_malloc(sizeof(DtedRecordData));

        m_records[fileIndex] = a;

        //
        // Read UHL
        //
        DtedReadUserHeaderLabel(fp, a);

        //
        // Read Data Set Identification (DSI) Record
        //
        DtedReadDataSetIdentificationRecord(fp, a);

        //
        // Read Accuracy Description (ACC) Record
        // ACC data are currently not used
        //
        DtedReadAccuracyDescriptionRecord(fp);

        //
        // Read elevation data
        //
        DtedReadElevationData(fp, a);

        fclose(fp);

        fileIndex++;
    }

    if (fileIndex == 0) {
        ERROR_ReportError("Cannot find DTED-FILENAME in the configuration file");
    }

    // set all remaining entries to NULL.
    for (i = fileIndex; i < MAX_NUM_DTED_FILES; i++) {
        m_records[i] = NULL;
    }

    if (DEBUG) {
        for (i = 0; i < fileIndex; i++) {
            printf("file %d, sw,ne = (%f,%f),(%f,%f)\n", i,
                   m_records[i]->southWestCorner.latlonalt.latitude,
                   m_records[i]->southWestCorner.latlonalt.longitude,
                   m_records[i]->northEastCorner.latlonalt.latitude,
                   m_records[i]->northEastCorner.latlonalt.longitude);
        }
    }

    m_numFiles = fileIndex;

    return;
}

double DtedTerrainData::getElevationAt(const Coordinates* point) {
    const  DtedRecordData* a;
    int    northWestLatitude;
    int    northWestLongitude;
    double normalizedLatitude;
    double normalizedLongitude;
    double dLatitude;
    double dLongitude;
    short  northWestElevation;
    short  northEastElevation;
    short  southWestElevation;
    short  southEastElevation;

    CoordinateType elevation;

    int bestFileIndex = findMatchingFile(point);

    if (bestFileIndex == DTED_NO_MATCH_FOUND) {
        if (m_terrainData->checkBoundaries())
        {
            char errorMessage[MAX_STRING_LENGTH];
             sprintf(errorMessage,
                    "Point (%lf, %lf) is not in the terrain database\n",
                    point->common.c1, point->common.c2);
            ERROR_ReportError(errorMessage);
        }
        return 0.0;
    }
    if (DEBUG) {
        printf("found matching DTED file %d\n", bestFileIndex);
    }

    m_mostRecentFile = bestFileIndex;

    a = m_records[bestFileIndex];
    assert(a != NULL);

    if (DEBUG) {
        printf("resolutions are %f,%f,%f\n",
               a->resolution[0],
               a->resolution[1],
               a->resolution[2]);
        fflush(stdout);
    }
    normalizedLongitude =
        ARC_SECONDS * (point->latlonalt.longitude -
         a->southWestCorner.latlonalt.longitude) / a->resolution[0];
    normalizedLatitude =
        ARC_SECONDS * (point->latlonalt.latitude -
         a->southWestCorner.latlonalt.latitude) / a->resolution[1];

    if (DEBUG) {
        printf("nlat,nlon = %f, %f\n", normalizedLatitude, normalizedLongitude);
        fflush(stdout);
    }
    northWestLatitude = (int) ceil(normalizedLatitude);
    northWestLongitude = (int) floor(normalizedLongitude);

    if (DEBUG) {
        printf("nwlat,nwlon = %d, %d\n", northWestLatitude, northWestLongitude);
        fflush(stdout);
    }
    dLatitude = northWestLatitude - normalizedLatitude;
    dLongitude = normalizedLongitude - northWestLongitude;

    if (DEBUG) {
        printf("dlat,dlon = %f, %f\n", dLatitude, dLongitude);
        fflush(stdout);
    }
    assert(dLatitude >= 0.0 && dLatitude < 1.0);

    assert(dLongitude >= 0.0 && dLongitude < 1.0);

    assert(northWestLongitude >= 0 &&
           northWestLongitude < a->numRows);

    assert(northWestLatitude >= 0 &&
           northWestLatitude < a->numColumns);

    northWestElevation =
        a->elevationData[northWestLongitude][northWestLatitude];

    if (northWestLongitude < a->numRows - 1 && northWestLatitude > 0) {
        southEastElevation =
            a->elevationData[northWestLongitude + 1][northWestLatitude - 1];
    }
    else {
        assert(dLatitude == 0.0 || dLongitude == 0.0);
        southEastElevation = 0;
    }

    if (dLatitude > dLongitude) {
        southWestElevation =
            a->elevationData[northWestLongitude][northWestLatitude - 1];

        elevation =
            northWestElevation +
            (southEastElevation - southWestElevation) * dLongitude +
            (southWestElevation - northWestElevation) * dLatitude;
    }
    else {
        if (northWestLongitude == a->numRows - 1) {
            northEastElevation =
                a->elevationData[northWestLongitude][northWestLatitude];
        }
        else {
            // northWestLongitude < a->numRows, from earlier assert()
            northEastElevation =
                a->elevationData[northWestLongitude + 1][northWestLatitude];
        }

        elevation =
            northWestElevation +
            (northEastElevation - northWestElevation) * dLongitude +
            (southEastElevation - northEastElevation) * dLatitude;
    }

    return elevation;
}

void DtedTerrainData::getHighestAndLowestForFile(
    DtedRecordData*    record,
    const Coordinates* sw,
    const Coordinates* ne,
    double*            highest,
    double*            lowest)
{
    for (int lon = 0; lon < record->numRows; lon++) {
        for (int lat = 0; lat < record->numColumns; lat++) {
            *highest = MAX(*highest, record->elevationData[lon][lat]);
            *lowest  = MIN(*lowest, record->elevationData[lon][lat]);
        }
    }
}

void DtedTerrainData::getElevationBoundaries(int index, Coordinates* sw, Coordinates* ne)
{
    ERROR_Assert(index >= 0 && index < m_numFiles, "Invalid index");

    *sw = m_records[index]->southWestCorner;
    *ne = m_records[index]->northEastCorner;
}

void DtedTerrainData::getHighestAndLowestElevation(
    const Coordinates* sw,
    const Coordinates* ne,
    double* highest,
    double* lowest)
{
    UInt32 i;
    bool foundOverlap = false;

    *highest = -99999.9;
    *lowest  = 99999.9;

    for (i = 0; i < m_numFiles; i++) {
        DtedRecordData* thisRecord = m_records[i];
        if (COORD_RegionsOverlap(m_terrainData->getCoordinateSystem(),
                                 sw, ne,
                                 &(thisRecord->southWestCorner),
                                 &(thisRecord->northEastCorner)))
    {
            foundOverlap = true;
            getHighestAndLowestForFile(thisRecord, sw, ne, highest, lowest);
        }
    }

    if (!foundOverlap) {
        // should probably do a bounds checking thing here
        *highest = 0.0;
        *lowest  = 0.0;
    }
}

UInt32 DtedTerrainData::findMatchingFile(const Coordinates* point) {
    // have to be careful with m_mostRecentFile to ensure thread safety

    UInt32 lastMatch = m_mostRecentFile; // make a local copy
    UInt32 i, bestFileIndex;
    bool   foundMatch = false;

    if (DEBUG) {
        printf("last match, point = %d, (%f,%f)\n", lastMatch,
               point->latlonalt.latitude,
               point->latlonalt.longitude);
    }
    // check for corrupted value
    if (lastMatch < m_numFiles) {
        DtedRecordData* thisRecord = m_records[lastMatch];
        if (DEBUG) {
            printf("record sw, ne = (%f,%f), (%f,%f)\n",
                   thisRecord->southWestCorner.latlonalt.latitude,
                   thisRecord->southWestCorner.latlonalt.longitude,
                   thisRecord->northEastCorner.latlonalt.latitude,
                   thisRecord->northEastCorner.latlonalt.longitude);
        }
        if (COORD_PointWithinRange(m_terrainData->getCoordinateSystem(),
                                   &(thisRecord->southWestCorner),
                                   &(thisRecord->northEastCorner),
                                   point))
{
            // this is still a good match
            if (DEBUG) printf("found match\n");
            return lastMatch;
        }
    }

    for (i = 0; i < m_numFiles; i++)
    {
        DtedRecordData* thisRecord = m_records[i];
        if (DEBUG) {
            printf("record sw, ne = (%f,%f), (%f,%f)\n",
                   thisRecord->southWestCorner.latlonalt.latitude,
                   thisRecord->southWestCorner.latlonalt.longitude,
                   thisRecord->northEastCorner.latlonalt.latitude,
                   thisRecord->northEastCorner.latlonalt.longitude);
        }
        if (COORD_PointWithinRange(m_terrainData->getCoordinateSystem(),
                                   &(thisRecord->southWestCorner),
                                   &(thisRecord->northEastCorner),
                                   point))
        {
            if (!foundMatch)
            {
                if (DEBUG) printf("found match\n");
                foundMatch = true;
                bestFileIndex = i;
            }
            else
            {
                // Found another file for the same position
                // if its of a better resolution use that one.
                if (m_records[bestFileIndex]->resolution[0] >
                        thisRecord->resolution[0] &&
                        m_records[bestFileIndex]->resolution[1] >
                        thisRecord->resolution[1])
                {
                    bestFileIndex = i;
    }
    }
        }
    }

    if (foundMatch)
    {
        m_mostRecentFile = bestFileIndex;

        return bestFileIndex;
    }
    else
    {
        // error handling is performed by calling function
        return DTED_NO_MATCH_FOUND;
    }
}

