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
#include "terrain_cartesian.h"

#define MAX_ELEVATION_STR 10

//
// CARTESIAN defines columns and rows in the following way.
//
//  #Rows #Columns Granularity
// SW Columns--->
//  R||||||||||
//  o||||||||||
//  w||||||||||
//  s||||||||||
//  |||||||||||
//  |||||||||||
//  |||||||||||
//    v|||||||||| NE

CartesianTerrainData::~CartesianTerrainData() {
    for (int r = 0; r < m_numRows; r++) {
        MEM_free(m_elevationData[r]);
    }
    MEM_free(m_elevationData);
}

void CartesianTerrainData::initialize(NodeInput *nodeInput) {
    int i;
    char terrainFilename[MAX_STRING_LENGTH];
    BOOL wasFound;
    FILE *fp;

    IO_ReadString(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "CARTESIAN-FILENAME",
        &wasFound,
        terrainFilename);

    if (wasFound != TRUE) {
        return;
    }

    fp = fopen(terrainFilename, "rb");

    if (fp == NULL) {
        char errorMessage[MAX_STRING_LENGTH];

        sprintf(errorMessage, "Cannot open CARTESIAN-FILENAME: %s",
                terrainFilename);
        ERROR_ReportError(errorMessage);
    }

    if (fscanf(fp, "%d %d %d", &m_numRows, &m_numColumns, &m_granularity) != 3)
    {
        char errorMessage[MAX_STRING_LENGTH];

        sprintf(errorMessage, "Error while reading CARTESIAN-FILENAME: %s",
                terrainFilename);
        ERROR_ReportError(errorMessage);
    }

    m_elevationData = (double**)MEM_malloc(m_numRows * sizeof(double*));
    assert(m_elevationData != NULL);

    for (i = 0; i < m_numRows; i++) {
        m_elevationData[i] =
            (double*)MEM_malloc(m_numColumns * sizeof(double));
        assert(m_elevationData[i] != NULL);
    }

    for (i = 0; i < m_numRows; i++) {
        int j;
        char elevation[MAX_ELEVATION_STR];

        for (j = 0; j < m_numColumns; j++) {

            if(fscanf(fp, "%s", elevation) != 1)
            {
                char errorMessage[MAX_STRING_LENGTH];

                sprintf(errorMessage, "Error while reading  CARTESIAN-FILENAME: %s",
                        terrainFilename);
                ERROR_ReportError(errorMessage);
            }
            m_elevationData[i][j] = atof(elevation);

        }
    }

    fclose(fp);

    m_sw.cartesian.x = 0;
    m_sw.cartesian.y = 0;

    m_ne.cartesian.x = (m_numColumns - 1) * m_granularity;
    m_ne.cartesian.y = (m_numRows - 1) * m_granularity;

    return;
}

double CartesianTerrainData::getElevationAt(const Coordinates* point)
{
    int    northWestX;
    int    northWestY;
    double dX;
    double dY;
    double  northWestElevation;
    double  northEastElevation;
    double  southWestElevation;
    double  southEastElevation;

    int rowsinY;
    int colsinX;

    double elevation;

    if (!COORD_PointWithinRange(CARTESIAN, &m_sw, &m_ne, point))
    {
        // could not find the map
        if (m_terrainData->checkBoundaries())
        {
            char errorMessage[MAX_STRING_LENGTH];

             sprintf(errorMessage,
                    "Position (%lf, %lf) is not in the terrain database\n",
                    point->common.c2, point->common.c1);
            ERROR_ReportError(errorMessage);
        }

        return 0.0;
    }

    northWestY = (int) floor(point->cartesian.y / m_granularity);
    northWestX = (int) floor(point->cartesian.x / m_granularity);

    dY = point->cartesian.y / m_granularity - northWestY;
    dX = point->cartesian.x / m_granularity - northWestX;

    assert(dX >= 0.0 && dX < 1.0);
    assert(dY >= 0.0 && dY < 1.0);

    assert(northWestY >= 0 && northWestY <= m_numRows);
    assert(northWestX >= 0 && northWestX <= m_numColumns);

    rowsinY = m_numRows;
    colsinX = m_numColumns;

    if(northWestY == rowsinY && northWestX == colsinX){

        northWestElevation = m_elevationData[northWestY - 1][northWestX - 1];
        southEastElevation = m_elevationData[northWestY - 1][northWestX - 1];
    }
    else if(northWestY == rowsinY && northWestX != colsinX){

        northWestElevation = m_elevationData[northWestY - 1][northWestX];
        southEastElevation = m_elevationData[northWestY - 1][northWestX + 1];
    }
    else if(northWestY != rowsinY && northWestX == colsinX){

        northWestElevation = m_elevationData[northWestY][northWestX - 1];
        southEastElevation = m_elevationData[northWestY + 1][northWestX - 1];
    }
    else{

        northWestElevation = m_elevationData[northWestY][northWestX];
        southEastElevation = m_elevationData[northWestY + 1][northWestX + 1];
    }

    if (dX > dY) {
        if(northWestY == rowsinY && northWestX == colsinX){
            southWestElevation = m_elevationData[northWestY - 1][northWestX - 1];
        }
        else if(northWestY == rowsinY && northWestX != colsinX){
            southWestElevation = m_elevationData[northWestY - 1][northWestX];
        }
        else if(northWestY != rowsinY && northWestX == colsinX){
            southWestElevation = m_elevationData[northWestY + 1][northWestX - 1];
        }
        else{
            southWestElevation = m_elevationData[northWestY + 1][northWestX];
        }
        elevation =
            northWestElevation +
            (southEastElevation - southWestElevation) * dX +
            (southWestElevation - northWestElevation) * dY;
    }
    else { //if dX <= dY
        if(northWestY == rowsinY && northWestX == colsinX){
            northEastElevation = m_elevationData[northWestY - 1][northWestX - 1];
        }
        else if(northWestY == rowsinY && northWestX != colsinX){
            northEastElevation = m_elevationData[northWestY - 1][northWestX + 1];
        }
        else if(northWestY != rowsinY && northWestX == colsinX){
            northEastElevation = m_elevationData[northWestY][northWestX - 1];
        }
        else{
            northEastElevation = m_elevationData[northWestY][northWestX + 1];
        }
        elevation =
            northWestElevation +
            (northEastElevation - northWestElevation) * dX +
            (southEastElevation - northEastElevation) * dY;
    }

    return elevation;
}

void CartesianTerrainData::getHighestAndLowestElevation(
    const Coordinates* sw,
    const Coordinates* ne,
    double* highest,
    double* lowest) {

    CoordinateType north, south, east, west;

    // first do sanity checking
    assert(sw->cartesian.x <= ne->cartesian.x);
    assert(sw->cartesian.y <= ne->cartesian.y);

    *highest = -99999.9;
    *lowest  = 99999.9;

    // set the search dimensions.
    north = MIN(ne->cartesian.y, m_ne.cartesian.y);
    south = MAX(sw->cartesian.y, m_sw.cartesian.y);
    east  = MIN(ne->cartesian.x, m_ne.cartesian.x);
    west  = MAX(sw->cartesian.x, m_sw.cartesian.x);

    int leftIndex   = (int) floor(west * m_numColumns / m_ne.cartesian.x);
    int rightIndex  = (int) ceil(east * m_numColumns / m_ne.cartesian.x);
    int topIndex    = (int) ceil(south * m_numRows / m_ne.cartesian.y);
    int bottomIndex = (int) floor(north * m_numRows / m_ne.cartesian.y);

    for (int r = bottomIndex; r < topIndex; r++) {
        for (int c = leftIndex; c < rightIndex; c++) {
            *highest = MAX(*highest, m_elevationData[r][c]);
            *lowest  = MIN(*lowest, m_elevationData[r][c]);
    }
    }
    }

