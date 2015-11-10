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
#include <limits.h>

#include <string>
#include <iostream>

#include "terrain.h"

#ifdef WIRELESS_LIB
#include "terrain_cartesian.h"
#include "terrain_dem.h"
#include "terrain_dted.h"
#include "terrain_qualnet_urban.h"
#endif // WIRELESS_LIB

#ifdef CTDB7_INTERFACE
#include "ctdb_7_interface.h"
#endif // CTDB7_INTERFACE

#ifdef CTDB8_INTERFACE
#include "ctdb_8_interface.h"
#endif // CTDB8_INTERFACE

#ifdef OOS_INTERFACE
#include "oos_interface.h"
#endif // OOS_INTERFACE

#ifdef MGRS_ADDON
#include "mgrs_utility.h"
#endif // MGRS_ADDON

#ifdef OTF_ADDON
#include "terrain_otf.h"
#endif // OTF_ADDON

#define DEBUG 1
#define NODEBUG 0

#define VALUE_NOT_USED -1


// Note: all region based code is a work in progress.

//  public domain function by Darel Rex Finley, 2006
//  Determines the intersection point of the line segment defined by points
//  A and B  with the line segment defined by points C and D.
//
//  Returns true if the intersection point was found, and stores that point
//  in X,Y.
//  Returns false if there is no determinable intersection point, in which
//  case X,Y will be unmodified.

static bool lineSegmentIntersection(const Coordinates* A,
                                    const Coordinates* B,
                                    const Coordinates* C,
                                    const Coordinates* D,
                                    Coordinates* intersection) {
    // TBD, this probably only works for Cartesian
    double  distAB, theCos, theSin, newX, ABpos;
    double  Ax, Bx, Cx, Dx, Ay, By, Cy, Dy;

    Ax = A->common.c1;
    Ay = A->common.c2;
    Bx = B->common.c1;
    By = B->common.c2;
    Cx = C->common.c1;
    Cy = C->common.c2;
    Dx = D->common.c1;
    Dy = D->common.c2;

    //  Fail if either line segment is zero-length.
    if (Ax == Bx &&
        Ay == By ||
        Cx == Dx &&
        Cy == Dy) {

        return false;
    }

    //  Fail if the segments share an end-point.
    if (Ax == Cx && Ay == Cy ||
        Bx == Cx && By == Cy ||
        Ax == Dx && Ay == Dy ||
        Bx == Dx && By == Dy) {

        return false;
    }

    //  (1) Translate the system so that point A is on the origin.
    Bx -= Ax;
    By -= Ay;
    Cx -= Ax;
    Cy -= Ay;
    Dx -= Ax;
    Dy -= Ay;

    //  Discover the length of segment A-B.
    distAB = sqrt(Bx * Bx + By * By);

    //  (2) Rotate the system so that point B is on the positive X axis.
    theCos = Bx / distAB;
    theSin = By / distAB;
    newX = Cx * theCos+Cy * theSin;
    Cy = Cy * theCos-Cx * theSin;
    Cx = newX;
    newX = Dx * theCos+Dy * theSin;
    Dy = Dy * theCos-Dx * theSin;
    Dx = newX;

    //  Fail if segment C-D doesn't cross line A-B.
    if (Cy < 0.0  && Dy < 0.0 ||
        Cy >= 0.0 && Dy >= 0.0) return false;

    // (3) Discover the position of the intersection point along line A-B.
    ABpos = Dx + (Cx - Dx) *
        Dy / (Dy - Cy);

    //  Fail if segment C-D crosses line A-B outside of segment A-B.
    if (ABpos < 0.0 || ABpos > distAB) return false;

    // (4) Apply the discovered position to line A-B in the original
    // coordinate system.
    intersection->common.c1 = Ax + ABpos * theCos;
    intersection->common.c2 = Ay + ABpos * theSin;

    //  Success.
    return true;
}

void TerrainRegion::print() const
{
    COORD_PrintCoordinates(&m_sw, m_coordinateSystemType);
    COORD_PrintCoordinates(&m_ne, m_coordinateSystemType);
    std::cout << "numBuildings         = " << m_numBuildings   << std::endl;
    std::cout << "maxElevation         = " << m_maxElevation << std::endl;
    std::cout << "minElevation         = " << m_minElevation  << std::endl;
    std::cout << "maxRoofHeight        = " << m_maxRoofHeight << std::endl;
    std::cout << "minRoofHeight        = " << m_minRoofHeight << std::endl;
    std::cout << "avgRoofHeight        = " << m_avgRoofHeight << std::endl;
    std::cout << "buildingCoverage     = " << m_buildingCoverage << std::endl;
    std::cout << "avgStreetWidth       = " << m_avgStreetWidth  << std::endl;
    std::cout << "avgCeilingHeight     = " << m_avgCeilingHeight << std::endl;
    std::cout << "avgWallSpacing       = " << m_avgWallSpacing << std::endl;
    std::cout << "avgWallThickness     = " << m_avgWallThickness << std::endl;
    std::cout << "percentageThickWalls = " << m_percentageThickWalls << std::endl;
    std::cout << "thickWallMaterial    = " << m_thickWallMaterial  << std::endl;
    std::cout << "thinWallMaterial     = " << m_thinWallMaterial  << std::endl;
    std::cout << "numFoliage           = " << m_numFoliage << std::endl; 
    std::cout << "foliageCoverage      = " << m_foliageCoverage  << std::endl; 
    std::cout << "avgFoliageHeight     = " << m_avgFoliageHeight << std::endl; 
    std::cout << "avgFoliageDensity    = " << m_avgFoliageDensity   << std::endl;
    std::cout << "typicalFoliatedState = " << m_typicalFoliatedState  << std::endl;
}

bool TerrainRegion::intersects(const Coordinates* c1,
                               const Coordinates* c2) const
{
    // Essentially, find any edge of the region that intersects the line

    Coordinates se;
    Coordinates nw;
    Coordinates cross;

    if (m_coordinateSystemType == CARTESIAN) {
        se.cartesian.x = m_ne.cartesian.x;
        se.cartesian.y = m_sw.cartesian.y;
        nw.cartesian.x = m_sw.cartesian.x;
        nw.cartesian.y = m_ne.cartesian.y;
    }
#ifdef MGRS_ADDON
    else if (m_coordinateSystemType == MGRS)
    {
        se.common.c1 = m_ne.common.c1;
        se.common.c2 = m_sw.common.c2;
        nw.common.c1 = m_sw.common.c1;
        nw.common.c2 = m_ne.common.c2;
    }
#endif // MGRS_ADDON
    else {
        se.latlonalt.longitude = m_ne.latlonalt.longitude;
        se.latlonalt.latitude  = m_sw.latlonalt.latitude;
        nw.latlonalt.longitude = m_sw.latlonalt.longitude;
        nw.latlonalt.latitude  = m_ne.latlonalt.latitude;
    }

    return (lineSegmentIntersection(c1, c2, &m_sw, &se, &cross) ||
            lineSegmentIntersection(c1, c2, &se, &m_ne, &cross) ||
            lineSegmentIntersection(c1, c2, &m_ne, &nw, &cross) ||
            lineSegmentIntersection(c1, c2, &nw, &m_sw, &cross));
}

bool TerrainRegion::intersects(const Box* box) const
{
    // The region intersects the box if both dimensions overlap.

    if (m_coordinateSystemType == CARTESIAN) {

        return ( // check X dimension
            (m_ne.cartesian.x >= box->getMinX()) &&
            (m_sw.cartesian.x <= box->getMaxX()) &&
            // check Y dimension
            (m_ne.cartesian.y >= box->getMinY()) &&
            (m_sw.cartesian.y <= box->getMaxY()));
    }
#ifdef MGRS_ADDON
    else if (m_coordinateSystemType == MGRS)
    {
        return // check X dimension
            m_ne.common.c1 >= box->getMinX() &&
            m_sw.common.c1 <= box->getMaxX() &&
            // check Y dimension
            m_ne.common.c2 >= box->getMinY() &&
            m_sw.common.c2 <= box->getMaxY();
    }
#endif // MGRS_ADDON
    else { // coordinateSystemType == LATLONALT

        // Does urban parser store box in Lon,Lat or Lat,Lon?

        return ( // check latitude
            (m_ne.latlonalt.latitude >= box->getMinY()) &&
            (m_sw.latlonalt.latitude <= box->getMaxY()) &&
            // check longitude
            (m_ne.latlonalt.longitude >= box->getMinX()) &&
            (m_sw.latlonalt.longitude <= box->getMaxX()));
    }
}

PathSegment TerrainRegion::getIntersect(const Coordinates* c1,
                                        const Coordinates* c2)
{
    // Essentially, find any edge of the region that intersects the line

    Coordinates se;
    Coordinates nw;
    Coordinates cross;
    PathSegment segment;

    if (m_coordinateSystemType == CARTESIAN) {
        se.cartesian.x = m_ne.cartesian.x;
        se.cartesian.y = m_sw.cartesian.y;
        nw.cartesian.x = m_sw.cartesian.x;
        nw.cartesian.y = m_ne.cartesian.y;
    }
#ifdef MGRS_ADDON
    else if (m_coordinateSystemType == MGRS)
    {
        se.common.c1 = m_ne.common.c1;
        se.common.c2 = m_sw.common.c2;
        nw.common.c1 = m_sw.common.c1;
        nw.common.c2 = m_ne.common.c2;
    }
#endif // MGRS_ADDON
    else {
        se.latlonalt.longitude = m_ne.latlonalt.longitude;
        se.latlonalt.latitude  = m_sw.latlonalt.latitude;
        nw.latlonalt.longitude = m_sw.latlonalt.longitude;
        nw.latlonalt.latitude  = m_ne.latlonalt.latitude;
    }

    // TBD, need to search the edges in order of closeness to c1.
    segment.setRegion(this);
    segment.setType(m_regionType);

    if (lineSegmentIntersection(c1, c2, &m_sw, &se, &cross)) {
        segment.setSource(&cross);
    }
    // TBD, need to check if one or both of the endpoints are inside
    // the region, then we can use them directly.

    return segment;
}

TERRAIN::ConstructionMaterials 
UrbanIndoorPathProperties::getMostCommonFloorMaterial() {
    int f, max;
    int materialCounts[7];
    TERRAIN::ConstructionMaterials mostCommon;

    for (f = 0; f < 7; f++) {
        materialCounts[f] = 0;
    }
    for (f = 0; f < m_numFloors; f++) {
        materialCounts[(int) m_floorMaterials[f]]++;
    }
    max = 0;
    for (f = 0; f < 7; f++) {
        if (materialCounts[f] > max) {
            max = materialCounts[f];
            mostCommon = (TERRAIN::ConstructionMaterials) f;
        }
    }
    return mostCommon;
}

TERRAIN::ConstructionMaterials 
UrbanIndoorPathProperties::getMostCommonThickWallMaterial() {
    int w, max;
    int materialCounts[7];
    TERRAIN::ConstructionMaterials mostCommon;

    for (w = 0; w < 7; w++) {
        materialCounts[w] = 0;
    }
    for (w = 0; w < m_numWalls; w++) {
        if (m_wallThicknesses[w] >= THIN_WALL_THICKNESS)
            materialCounts[(int) m_wallMaterials[w]]++;
    }
    max = 0;
    for (w = 0; w < 7; w++) {
        if (materialCounts[w] > max) {
            max = materialCounts[w];
            mostCommon = (TERRAIN::ConstructionMaterials) w;
        }
    }
    return mostCommon;
}

TERRAIN::ConstructionMaterials 
UrbanIndoorPathProperties::getMostCommonThinWallMaterial() {
    int w, max;
    int materialCounts[7];
    TERRAIN::ConstructionMaterials mostCommon;

    for (w = 0; w < 7; w++) {
        materialCounts[w] = 0;
    }
    for (w = 0; w < m_numWalls; w++) {
        if (m_wallThicknesses[w] < THIN_WALL_THICKNESS)
            materialCounts[(int) m_wallMaterials[w]]++;
    }
    max = 0;
    for (w = 0; w < 7; w++) {
        if (materialCounts[w] > max) {
            max = materialCounts[w];
            mostCommon = (TERRAIN::ConstructionMaterials) w;
        }
    }
    return mostCommon;
}

double UrbanIndoorPathProperties::getAvgFloorThickness() {
    int f;
    double total = 0.0;

    for (f = 0; f < m_numFloors; f++) {
        total += m_floorThicknesses[f];
    }
    return (total / m_numFloors);
}

double UrbanIndoorPathProperties::getAvgWallThickness() {
    int w;
    double total = 0.0;

    for (w = 0; w < m_numWalls; w++) {
        total += m_wallThicknesses[w];
    }
    return (total / m_numWalls);
}

double UrbanIndoorPathProperties::getAvgThickWallThickness() {
    int w;
    double total = 0.0;

    for (w = 0; w < m_numWalls; w++) {
        if (m_wallThicknesses[w] >= THIN_WALL_THICKNESS)
            total += m_wallThicknesses[w];
    }
    return (total / m_numThickWalls);
}

double UrbanIndoorPathProperties::getAvgThinWallThickness() {
    int w;
    double total = 0.0;

    for (w = 0; w < m_numWalls; w++) {
        if (m_wallThicknesses[w] < THIN_WALL_THICKNESS)
            total += m_wallThicknesses[w];
    }
    return (total / m_numThinWalls);
}


int ElevationTerrainData::getElevationArray(
    const Coordinates* c1,
    const Coordinates* c2,
    const double       distance,
    const double       samplingDistance,
    double             elevationArray[],
    const int          maxSamples) {

    int numSamples;
    double d1, d2;
    Coordinates position;
    int i;

    // actual # of samples is numSamples + 1 as we include samples at
    // both end. Thus "maxSamples - 1" is used below to avoid access
    // memory issue
    numSamples =
        MIN((int)ceil(distance / samplingDistance), maxSamples - 1);
    
    d1 = (c2->common.c1 - c1->common.c1) / (double)numSamples;
    if (m_terrainData->getCoordinateSystem()  ==  LATLONALT) {
        d2 = COORD_LongitudeDelta(c2->common.c2, c1->common.c2);
    }
    else {
        d2 = c2->common.c2 - c1->common.c2;
    }
    d2 /= (double)numSamples;

    //
    // This if statement is added to return the same array
    // for the given two end-points regardless of the direction
    //
    if (c1->common.c1 >= c2->common.c1)
    {
        position.common.c1 = c1->common.c1;
        position.common.c2 = c1->common.c2;
    }
    else {
        position.common.c1 = c2->common.c1;
        position.common.c2 = c2->common.c2;
        d1 = -d1;
        d2 = -d2;
    }

    if (NODEBUG) {
        printf("distance: %lf\n", distance);
        printf("numSamples: %d\n", numSamples);
    }

    for (i = 0; i <= numSamples; i++) {
        elevationArray[i] = getElevationAt(&position);

        if (NODEBUG) {
            printf("%d (%lf, %lf, %lf)\n",
                   i,
                   position.common.c1,
                   position.common.c2,
                   elevationArray[i]);
        }
        position.common.c1 += d1;
        position.common.c2 += d2;
    }

    return numSamples;
}

void TerrainData::initialize(NodeInput* nodeInput,
                             bool       masterProcess) {

    BOOL wasFound;
    char buf[MAX_STRING_LENGTH];

    //
    // Set the default values
    //
    m_coordinateSystemType = CARTESIAN;
    m_sw.common.c1 = 0.0;
    m_sw.common.c2 = 0.0;
    m_sw.common.c3 = 0.0;
    m_ne.common.c1 = 0.0;
    m_ne.common.c2 = 0.0;
    m_ne.common.c3 = 0.0;
    m_dimensions.common.c1 = 0.0;
    m_dimensions.common.c2 = 0.0;
    m_dimensions.common.c3 = 0.0;

    m_boundaryCheck = true;

    //
    // Initialize terrain database if necessary
    //
    IO_ReadString(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "TERRAIN-DATA-TYPE",
        &wasFound, buf);

    if (wasFound) {
        if (strcmp(buf, "DEM") == 0) {
            m_coordinateSystemType = LATLONALT;
            m_elevationData = new DemTerrainData(this);
            m_elevationData->initialize(nodeInput);
        }
        else if (strcmp(buf, "DTED") == 0) {
            m_coordinateSystemType = LATLONALT;
            m_elevationData = new DtedTerrainData(this);
            m_elevationData->initialize(nodeInput);
        }
        else if (strcmp(buf, "CARTESIAN") == 0) {
            m_coordinateSystemType = CARTESIAN;
            m_elevationData = new CartesianTerrainData(this);
            m_elevationData->initialize(nodeInput);
        }
        else
#ifdef CTDB7_INTERFACE
        if (strcmp(buf, "CTDB7") == 0) {
            m_coordinateSystemType = LATLONALT;
            m_elevationData = new Ctdb7TerrainData(this);
            m_elevationData->initialize(nodeInput);
        }
        else
#endif // CTDB7_INTERFACE
#ifdef CTDB8_INTERFACE
        if (strcmp(buf, "CTDB8") == 0) {
            m_coordinateSystemType = LATLONALT;
            m_elevationData = new Ctdb8TerrainData(this);
            m_elevationData->initialize(nodeInput);
        }
        else
#endif // CTDB8_INTERFACE
#ifdef OOS_INTERFACE
        if (strcmp(buf, "OTF") == 0) {
            m_coordinateSystemType = LATLONALT;
            m_elevationData = new OtfElevationTerrainData(this);
            m_elevationData->initialize(nodeInput);
            // we'll also initialize this in the urban section below.
        }
        else
#endif // OOS_INTERFACE
#ifdef OTF_ADDON
        if (strcmp(buf, "OTF") == 0)
        {
            m_coordinateSystemType = LATLONALT;
            m_elevationData = new OtfTerrainData(this);
            m_elevationData->initialize(nodeInput);
        }
        else
#endif // OTF_ADDON
        if (strcmp(buf, "NONE") == 0) {
            m_elevationData = new ElevationTerrainData(this);
        }
        else {
            ERROR_ReportError("Unknown TERRAIN-DATA-TYPE\n");
        }

        IO_ReadString(
            ANY_NODEID,
            ANY_ADDRESS,
            nodeInput,
            "TERRAIN-DATA-BOUNDARY-CHECK",
            &wasFound, buf);

        if (wasFound) {
            if (strcmp(buf, "YES") == 0) {
                m_boundaryCheck = true;
                }
                else if (strcmp(buf, "NO") == 0) {
                m_boundaryCheck = false;
                }
                else {
                ERROR_ReportWarning("Unrecognized input for "
                                    "TERRAIN-DATA-BOUNDARY-CHECK, "
                                    "using default.");
            }
        }
    }
    else {
        m_elevationData = new ElevationTerrainData(this);
    }

    IO_ReadString(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "COORDINATE-SYSTEM",
        &wasFound,
        buf);

    if (wasFound) {
        if (strcmp(buf, "CARTESIAN") == 0) {
            if (m_coordinateSystemType == LATLONALT) {
                ERROR_ReportError(
                    "COORDINATE-SYSTEM must be LATLONALT "
                    "for the specified TERRAIN-DATA-TYPE\n");
            }
        }
        else if (strcmp(buf, "LATLONALT") == 0) {
            m_coordinateSystemType = LATLONALT;
        }
#ifdef MGRS_ADDON
        else if (strcmp(buf, "MGRS") == 0)
        {
            if (m_coordinateSystemType == LATLONALT)
            {
                ERROR_ReportError("COORDINATE-SYSTEM must be LATLONALT for "
                    "the specified TERRAIN-DATA-TYPE\n");
            }
            else
            {
                m_coordinateSystemType = MGRS;
            }
        }
#endif // MGRS_ADDON
        else {
            char errorStr[MAX_STRING_LENGTH];

            sprintf(errorStr, "\"COORDINATE-SYSTEM %s\" in the configuration "
                    "file is invalid.\nCOORDINATE-SYSTEM should be either "
                    "\"CARTESIAN\" or \"LATLONALT\"", buf);

            ERROR_ReportError(errorStr);
        }
    }

    if (m_coordinateSystemType == CARTESIAN) {
        IO_ReadString(ANY_NODEID,
            ANY_ADDRESS,
            nodeInput,
            "TERRAIN-DIMENSIONS",
            &wasFound,
            buf);

        if (wasFound == TRUE) {
            COORD_ConvertToCoordinates(buf, &(m_ne));
        }
    }
#ifdef MGRS_ADDON
    else if (m_coordinateSystemType == MGRS)
    {
        MgrsUtility mgrs;

        // SW corner
        IO_ReadString(ANY_NODEID, ANY_ADDRESS, nodeInput,
            "MGRS-SOUTH-WEST-CORNER", &wasFound, buf);
        if (wasFound)
        {
            if (mgrs.isMgrs(buf))
            {
                m_mgrsSw = buf;
                m_sw.common.c1 = mgrs.easting(m_mgrsSw);
                m_sw.common.c2 = mgrs.northing(m_mgrsSw);
            }
            else
            {
                ERROR_ReportError("'MGRS-SOUTH-WEST-CORNER' needs to be "
                "specified in the configuration file\nwith a value in MGRS "
                "format.");
            }
        }
        else
        {
            ERROR_ReportError("'MGRS-SOUTH-WEST-CORNER' needs to be "
                "specified in the configuration file\nwith a value in MGRS "
                "format.");
        }

        // NE corner
        IO_ReadString(ANY_NODEID, ANY_ADDRESS, nodeInput,
            "MGRS-NORTH-EAST-CORNER", &wasFound, buf);
        if (wasFound)
        {
            if (mgrs.isMgrs(buf))
            {
                if (mgrs.isInSameRegion(m_mgrsSw, buf))
                {
                    if (mgrs.isSouthWestOf(m_mgrsSw, buf))
                    {
                        m_mgrsNe = buf;
                        m_ne.common.c1 =
                            mgrs.swNeRectRightEdge(m_mgrsSw, buf);
                        m_ne.common.c2 =
                            mgrs.swNeRectTopEdge(m_mgrsSw, buf);
                    }
                    else
                    {
                        char errorStr[MAX_STRING_LENGTH];
                        sprintf(errorStr, "MGRS-NORTH-EAST-CORNER (%s) is "
                            "not northeast of\nMGRS-SOUTH-WEST-CORNER (%s)."
                            , buf, m_mgrsSw.c_str());
                        ERROR_ReportError(errorStr);
                    }
                }
                else
                {
                    char errorStr[MAX_STRING_LENGTH];
                    sprintf(errorStr, "MGRS-NORTH-EAST-CORNER (%s) and "
                        "MGRS-SOUTH-WEST-CORNER (%s) are not in the same "
                        "region.\nBoth must be the MGRS north pole, south "
                        "pole, or UTM.", buf, m_mgrsSw.c_str());
                    ERROR_ReportError(errorStr);
                }
            }
            else
            {
                ERROR_ReportError("'MGRS-SOUTH-WEST-CORNER' needs to be "
                "specified in the configuration file\nwith a value in MGRS "
                "format.");
            }
        }
        else
        {
            ERROR_ReportError("'MGRS-SOUTH-WEST-CORNER' needs to be "
                "specified in the configuration file\nwith a value in MGRS "
                "format.");
        }
    }
#endif // MGRS_ADDON
    else {
        IO_ReadString(
            ANY_NODEID,
            ANY_ADDRESS,
            nodeInput,
            "TERRAIN-SOUTH-WEST-CORNER",
            &wasFound,
            buf);

        if (wasFound) {
            COORD_ConvertToCoordinates(buf, &m_sw);
        }
        else {
            ERROR_ReportError(
                "\"TERRAIN-SOUTH-WEST-CORNER\" needs to be "
                "specified in the configuration file\nwith a value in "
                "(lat, lon) or (lat, lon, alt) format.");
        }

        IO_ReadString(
            ANY_NODEID,
            ANY_ADDRESS,
            nodeInput,
            "TERRAIN-NORTH-EAST-CORNER",
            &wasFound,
            buf);

        if (wasFound) {
            COORD_ConvertToCoordinates(buf, &m_ne);

            if (m_ne.latlonalt.latitude < m_sw.latlonalt.latitude) {
                char errorStr[MAX_STRING_LENGTH];

                sprintf(errorStr,
                        "TERRAIN-NORTH-EAST-CORNER (%f, %f) is located "
                        "south of\nTERRAIN-SOUTH-WEST-CORNER (%f, %f)."
                        "\nSo %f should have been greater than %f",
                        m_ne.latlonalt.latitude,
                        m_ne.latlonalt.longitude,
                        m_sw.latlonalt.latitude,
                        m_sw.latlonalt.longitude,
                        m_ne.latlonalt.latitude,
                        m_sw.latlonalt.latitude);

                ERROR_ReportError(errorStr);
            }
            }
        else {
            ERROR_ReportError(
                "\"TERRAIN-NORTH-EAST-CORNER\" needs to be "
                "specified in the configuration file\nwith a value in "
                "(lat, lon) or (lat, lon, alt) format.");
        }
    }

    calculateDimensions();

    //
    // Initialize urban terrain if necessary
    //
    IO_ReadString(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
        "URBAN-TERRAIN-TYPE",
        &wasFound, buf);

    if (wasFound) {
        if (strcmp(buf, "QUALNET-URBAN-TERRAIN") == 0) {
            m_urbanData = new QualNetUrbanTerrainData(this);
            m_urbanData->initialize(nodeInput, masterProcess);
            }
            else
#ifdef OOS_INTERFACE
        if (strcmp(buf, "OTF") == 0) {
            m_urbanData = new OtfUrbanTerrainData(this);
            m_urbanData->initialize(nodeInput, masterProcess);
        }
        else
#endif // OOS_INTERFACE
        if ((strcmp(buf, "NONE") == 0) || (strcmp(buf, "none") == 0)) {
            m_urbanData = new UrbanTerrainData(this);
    }
        else {
            ERROR_ReportError("Unknown URBAN-TERRAIN-TYPE\n");
    }
}
    else {
        m_urbanData = new UrbanTerrainData(this);
        }

    IO_ReadString(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "TERRAIN-USE-REGIONS",
        &wasFound,
        buf);

    if (wasFound) {
        if (!strcmp(buf, "YES")) {
            m_useRegions = true;
        }
        }
    if (m_useRegions) {
        initializeRegions(nodeInput);
        }

    if (NODEBUG) {
        print();
        }
        }

void TerrainData::finalize() {

    if (m_elevationData != NULL)
    {
        m_elevationData->finalize();
        delete m_elevationData;
        m_elevationData = NULL;
        }
    if (m_urbanData != NULL)
    {
        m_urbanData->finalize();
        delete m_urbanData;
        m_urbanData = NULL;
    }
    if (m_regions != NULL) {
        delete m_regions;
        m_regions = NULL;
}

}

void TerrainData::boundCoordinatesToTerrain(Coordinates* point,
                                            bool printWarnings) {

    if (point->common.c1 < m_sw.common.c1)
    {
        if (printWarnings)
        {
            ERROR_ReportWarning("coordinate 1 out of range (too small)");
        }

        point->common.c1 = m_sw.common.c1;
    }
    else if (point->common.c1 > m_ne.common.c1)
    {
        if (printWarnings)
        {
            ERROR_ReportWarning("coordinate 1 out of range (too large)");
        }

        point->common.c1 = m_ne.common.c1;
    }

    if (m_coordinateSystemType == LATLONALT) {
        // Longitude is a weird thing, because it wraps around from 180 to
        // -180.  If a point is outside the range, it is both east and
        // west of the terrain, so we need to adjust it to the closer
        // edge.

        CoordinateType longitude = point->common.c2;
        assert((longitude >= -180.0) && (longitude <= 180.0));

        CoordinateType west     = m_sw.common.c2;
        CoordinateType east     = m_ne.common.c2;
        CoordinateType distWest = west - longitude;
        CoordinateType distEast = longitude - east;

        if (east < west) {

            // sw is west of the dateline, ne is east of it.
            // in this case, we have to adjust the point's longitude to be
            // east of the origin, then we'll normalize back.

            if (((longitude >= west)   && (longitude <= 180.0)) ||
                ((longitude >= -180.0) && (longitude <= east)))
                return; // do nothing, in range
        }
        else // this is the easy case, w < e
        {
            if ((longitude >= west) && (longitude <= east))
                return; // do nothing

        }

        // either long < w or > e, find closer edge
        if (longitude < west)
           distEast += 360.0;
        else // longitude > east
           distWest += 360.0;

        if (distWest < distEast) {
            // set to west
            if (printWarnings)
            {
                ERROR_ReportWarning("longitude out of range (too far west)");
            }

            point->common.c2 = m_sw.common.c2;
        }
        else
        {
            // set to east
            if (printWarnings)
            {
                ERROR_ReportWarning("longitude out of range (too far east)");
            }

            point->common.c2 = m_ne.common.c2;
        }
    
        COORD_NormalizeLongitude(point);
    }
    else // CARTESIAN/MGRS
    {
        if (point->common.c2 < m_sw.common.c2)
        {
            if (printWarnings)
            {
                ERROR_ReportWarning("coordinate 2 out of range (too small)");
            }

            point->common.c2 = m_sw.common.c2;
        }
        else if (point->common.c2 > m_ne.common.c2)
        {
            if (printWarnings)
            {
                ERROR_ReportWarning("coordinate 2 out of range (too large)");
            }

            point->common.c2 = m_ne.common.c2;
        }
    }
}

// Path functions
UrbanIndoorPathProperties* TerrainData::getIndoorPathProperties(
     const Coordinates* c1,
     const Coordinates* c2) {

    return m_urbanData->getIndoorPathProperties(c1, c2, 0.0);
        }

UrbanPathProperties* TerrainData::getUrbanPathProperties(
    const  Coordinates* c1,
    const  Coordinates* c2,
    double pathWidth,
    bool   includeFoliage) {

    if (NODEBUG)
        std::cout << "UrbanPathProperties::getUrbanPathProperties..." 
                  << std::endl;

    return m_urbanData->getUrbanPathProperties(c1, c2, pathWidth,
                                               includeFoliage);
    }

void TerrainData::findEdgeOfBuilding(const Coordinates* indoorPoint,
                                     const Coordinates* outdoorPoint,
                                     Coordinates*       edgePoint) {


    // call the edge of building function (if there is one it will return true)
    if (m_urbanData && 
       m_urbanData->getEdgeOfBuilding(indoorPoint, outdoorPoint, edgePoint)) {

        // Check if edge point is in building
        bool isEdgeIndoors = m_urbanData->isPositionIndoors(edgePoint);
        if (isEdgeIndoors)
            printf("Edge is inside building\n");
        else
            printf("Edge is NOT INSIDE building\n");

        // Check if the to points are in the same building
        bool inSamebuilding = m_urbanData->arePositionsInSameBuilding(
                                              indoorPoint, 
                                              edgePoint);
        if (inSamebuilding)
            printf("Both points are in same building\n");
        else
            printf("Both points are NOT IN SAME building\n");

        //return; // Drop through so the debug statment can execute.

    } else {

    // temporary hack
    double latdiff;
    double londiff;
    double diffpct = 0.1;

        latdiff = outdoorPoint->common.c1 - indoorPoint->common.c1;
        londiff = outdoorPoint->common.c2 - indoorPoint->common.c2;

        // assume the signal goes out through the nearest wall
        edgePoint->common.c1 = indoorPoint->common.c1;
        edgePoint->common.c2 = indoorPoint->common.c2;
        edgePoint->common.c3 = indoorPoint->common.c3;

    do {
            edgePoint->common.c1 += (latdiff * diffpct);
            edgePoint->common.c2 += (londiff * diffpct);
        } while (arePositionsInSameBuilding(indoorPoint, edgePoint));
        }

//  SEB - Remove this after testing
    if (NODEBUG) {
        printf("inside, outside, edge are (%f,%f,%f) (%f,%f,%f) (%f,%f,%f)\n",
               indoorPoint->common.c1,
               indoorPoint->common.c2,
               indoorPoint->common.c3,
               outdoorPoint->common.c1,
               outdoorPoint->common.c2,
               outdoorPoint->common.c3,
               edgePoint->common.c1,
               edgePoint->common.c2,
               edgePoint->common.c3);
    }
}


std::list<PathSegment> TerrainData::getPathSegments(
    const Coordinates* c1,
    const Coordinates* c2) {

    // TBD
    // need a function for whether a line crosses a region
    // need a function for entry/exit points where line crosses region
    std::list<PathSegment> *newList = new std::list<PathSegment>();
    newList->clear();

    // shortcut the search by finding the regions containing c1 and c2
    // row1,column1 = findRegion(c1);
    // row2,column2 = findRegion(c2);
    // then go from min(row1,row2) to max(row1,row2)
    //     and from min(col1,col2) to max(col1,col2)
    // and check for crossing.

    int row1, col1;
    int row2, col2;
    int r, c;

    CoordinateType lat1, lon1, lat2, lon2;

    lat1 = c1->latlonalt.latitude;
    lon1 = c1->latlonalt.longitude;
    lat2 = c2->latlonalt.latitude;
    lon2 = c2->latlonalt.longitude;

    row1 = (int) floor(((lat1 - m_sw.latlonalt.latitude) * m_gridRows) /
                       m_dimensions.latlonalt.latitude);
    row2 = (int) floor(((lat2 - m_sw.latlonalt.latitude) * m_gridRows) /
                       m_dimensions.latlonalt.latitude);

    CoordinateType delta1, delta2;
    delta1 = lon1 - m_sw.latlonalt.longitude;
    delta2 = lon2 - m_sw.latlonalt.longitude;
    if (delta1 < 0.0) delta1 += 360.0;
    if (delta2 < 0.0) delta2 += 360.0;

    col1 = (int) floor((delta1 * m_gridCols) / 
                       m_dimensions.latlonalt.longitude);
    col2 = (int) floor((delta2 * m_gridCols) / 
                       m_dimensions.latlonalt.longitude);

    int deltaR = (row1 <= row2) ? 1 : -1;
    int deltaC = (col1 <= col2) ? 1 : -1;

    for (r = row1; r != (row2 + deltaR); r += deltaR) {
        for (c = col1; c != (col2 + deltaC); c += deltaC) {
            TerrainRegion* region = &(m_regions[r * m_gridRows + c]);
            if (region->intersects(c1, c2)) {
                newList->push_back(region->getIntersect(c1, c2));
            }
        }
    }

    return *newList;
}



// Utilities
std::string* TerrainData::terrainFileList(NodeInput* nodeInput, bool isUrbanIncluded) {

    std::string* response = new std::string();
    BOOL wasFound = FALSE;
    char terrainDataType[MAX_STRING_LENGTH];

    *response = "";

    IO_ReadString(ANY_NODEID,
                  ANY_ADDRESS,
                  nodeInput,
                  "TERRAIN-DATA-TYPE",
                  &wasFound,
                  terrainDataType);

    if (wasFound && (!strcmp(terrainDataType, "DEM") ||
                    !strcmp(terrainDataType, "DTED") ||
                    !strcmp(terrainDataType, "CARTESIAN")))
    {
        char terrainFileParamName[MAX_STRING_LENGTH];
        char terrainFileName[MAX_STRING_LENGTH];

        *response += terrainDataType;

        strcpy(terrainFileParamName, terrainDataType);
        strcat(terrainFileParamName, "-FILENAME");

        int i;

        for (i = 0;; i++)
        {
            IO_ReadStringInstance(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                terrainFileParamName,
                i,
                (i == 0),
                &wasFound,
                terrainFileName);

            if (!wasFound)
            {
                break;
            }
            else
            {
                *response += " ";
                *response += terrainFileName;
            }
        }
    }

#ifdef OTF_ADDON
    if (wasFound && !strcmp(terrainDataType, "OTF"))
    {
        char dbPath[MAX_STRING_LENGTH];

        *response += terrainDataType;

        IO_ReadString(ANY_NODEID, ANY_ADDRESS, nodeInput,
            "OTF-DATABASE-PATH", &wasFound, dbPath);
        if (wasFound)
        {
            *response += " \"";
            *response += dbPath;
            *response += "\"";
        }
    }
#endif // OTF_ADDON

    if (isUrbanIncluded)
    {
        std::string* urbanFiles = m_urbanData->fileList(nodeInput);
        *response += *urbanFiles;
        delete urbanFiles;
    }

    return response;
}

void TerrainData::calculateNE() {
    // implicit that we have SW and dimensions
    m_ne.common.c1 = m_sw.common.c1 + m_dimensions.common.c1;
    m_ne.common.c2 = m_sw.common.c2 + m_dimensions.common.c2;

    if (m_coordinateSystemType == LATLONALT) {
        COORD_NormalizeLongitude(&m_ne);
    }
}

void TerrainData::calculateDimensions() {
    // implicit that we have SW and NE
    m_dimensions.common.c1 = m_ne.common.c1 - m_sw.common.c1;
    m_dimensions.common.c2 = m_ne.common.c2 - m_sw.common.c2;

    if (m_coordinateSystemType == LATLONALT) {
        if (m_dimensions.common.c2 < 0.0) {
            m_dimensions.common.c2 += 360.0;
        }
    }
}

void UrbanRegionPathProperties::initialize() {
    // TBD
    // We need to calculate a couple things.  One is how many buildings
    // are on the path, which we estimate by the length of the path and
    // the number/spacing of buildings in the region. Similarly for foliage.
    // Second is the average size of a building/foliage.  We assume buildings
    // and foliage are square and therefore the width is the sqrt of the
    // size.

    int numBuildings = 0;
    }

void UrbanRegionPathProperties::print() {
    std::cout << "Printing path properties" << std::endl;
    std::cout << "  numBuildings = "               << m_numBuildings 
              << std::endl;
    std::cout << "  avgBuildingSeparation = "      << m_avgBuildingSeparation 
              << std::endl;
    std::cout << "  avgBuildingHeight = "          << m_avgBuildingHeight 
              << std::endl;
    std::cout << "  srcDistanceToIntersection = "  
              << m_srcDistanceToIntersection << std::endl;
    std::cout << "  destDistanceToIntersection = " 
              << m_destDistanceToIntersection << std::endl;
    std::cout << "  intersectionAngle = "          << m_intersectionAngle 
              << std::endl;
    std::cout << "  avgFoliageHeight = "           << m_avgFoliageHeight 
              << std::endl;
    std::cout << "  maxRoofHeight = "              << m_maxRoofHeight 
              << std::endl;
    std::cout << "  minRoofHeight = "              << m_minRoofHeight 
              << std::endl << std::endl;
    //TBD, add vectors
}

void TerrainData::initializeRegions(NodeInput* nodeInput) {

    int r, c;
    BOOL found;

    Coordinates regionSW;
    Coordinates regionNE;

    CoordinateType delta1; // x or lat
    CoordinateType delta2; // y or long

    IO_ReadInt(
          ANY_NODEID,
          ANY_ADDRESS,
          nodeInput,
          "TERRAIN-REGION-NUM-ROWS",
          &found,
          &m_gridRows);

    IO_ReadInt(
           ANY_NODEID,
           ANY_ADDRESS,
           nodeInput,
           "TERRAIN-REGION-NUM-COLUMNS",
           &found,
           &m_gridCols);

    assert(m_gridRows > 0);
    assert(m_gridCols > 0);

    delta1 = (m_ne.common.c1 - m_sw.common.c1) / ((CoordinateType)m_gridRows);
    if (m_coordinateSystemType == LATLONALT) {
        delta2 = COORD_LongitudeDelta(m_ne.common.c2, m_sw.common.c2);
    }
    else { // CARTESIAN/MGRS
        delta2 = (m_ne.common.c2 - m_sw.common.c2);
    }
    delta2 /= (CoordinateType)m_gridCols;

    m_regions = new TerrainRegion[m_gridRows * m_gridCols];
    for (r = 0; r < m_gridRows; r++) {
        // set SW corner
        regionSW.common.c1 = m_sw.common.c1;
        regionSW.common.c2 = m_sw.common.c2 + (delta2 * r);
        if (m_coordinateSystemType == LATLONALT) {
            COORD_NormalizeLongitude(&regionSW);
        }

        // for each column
        for (c = 0; c < m_gridCols; c++) {
            // set the NE corner
            regionNE.common.c1 = regionSW.common.c1 + delta1;
            regionNE.common.c2 = regionSW.common.c2 + delta2;
            if (m_coordinateSystemType == LATLONALT) {
                COORD_NormalizeLongitude(&regionNE);
            }

            int index = (r * m_gridCols) + c;
            m_regions[index].setCoordinateSystem(m_coordinateSystemType);
            m_regions[index].setSW(&regionSW);
            m_regions[index].setNE(&regionNE);
            m_regions[index].calculateArea();

            m_urbanData->populateRegionData(&(m_regions[index]));

            regionSW.common.c1 += delta1;
        }
    }

    return;
}

bool TerrainData::getIsCanyon(const Coordinates* point,
                              const double       effectiveHeight) {
    // Effective height is the height above ground, including antenna.
    // A node is in a canyon if its effective height is lower than the
    // surrounding buildings.  So what we want is to find the buildings
    // in the ~100m square around the building.
    TerrainRegion region;
    Coordinates regionSW;
    Coordinates regionNE;

    // 0.0003 degrees is ~= 33.4 meters in latitude.  at most latitudes,
    // this will be > 25 meters longitude.
    const double regionDegrees = 0.0009;
    const double regionMeters  = 100.0;

    // TBD, if we're in abstract mode, we want to check the containing
    // region for type, building density and building height.

    if (m_coordinateSystemType == LATLONALT) {
        regionSW.latlonalt.latitude  = 
            point->latlonalt.latitude - regionDegrees;
        regionSW.latlonalt.longitude = 
            point->latlonalt.longitude - regionDegrees;
        regionNE.latlonalt.latitude  = 
            point->latlonalt.latitude + regionDegrees;
        regionNE.latlonalt.longitude = 
            point->latlonalt.longitude + regionDegrees;
    }
#ifdef MGRS_ADDON
    else if (m_coordinateSystemType == MGRS)
    {
        regionSW.common.c1 = MAX(0.0, (point->common.c1 - regionMeters));
        regionSW.common.c2 = MAX(0.0, (point->common.c2 - regionMeters));
        regionNE.common.c1 = MIN(m_ne.common.c1,
            (point->common.c1 + regionMeters));
        regionNE.common.c2 = MIN(m_ne.common.c2,
            (point->common.c2 + regionMeters));
    }
#endif
    else { // Cartesian
        regionSW.cartesian.x = MAX(0.0, (point->cartesian.x - regionMeters));
        regionSW.cartesian.y = MAX(0.0, (point->cartesian.y - regionMeters));
        regionNE.cartesian.x = MIN(m_ne.cartesian.x,
                                   (point->cartesian.x + regionMeters));
        regionNE.cartesian.y = MIN(m_ne.cartesian.y,
                                   (point->cartesian.y + regionMeters));
    }
    regionSW.common.c3 = 0.0;
    regionNE.common.c3 = 0.0;

    region.setCoordinateSystem(m_coordinateSystemType);
    region.setSW(&regionSW);
    region.setNE(&regionNE);
    region.calculateArea();

    // we must have urban data or what's the point of calling this function.
    m_urbanData->populateRegionData(&region, TerrainRegion::SIMPLE);
    if (NODEBUG) {
        printf("checking canyon for height %f\n", effectiveHeight);
        region.print();
    }

    // Our current definition of Canyon is that there's more than one
    // building in the immediate area, and those buildings are taller than
    // the node's antenna.  Assumes the node is not inside a building.
    return ((region.getNumBuildings() > 1) &&
            (region.getAvgRoofHeight() > effectiveHeight));
}

bool TerrainData::getFreeSpace(const Coordinates* point1,
                               const Coordinates* point2,
                               const double fresnelWidth) {
    // TBD, this function should check both elevation and urban data.
    return false;
    }

void TerrainData::getMinMaxBuildings(const Coordinates* c1,
                                     const Coordinates* c2,
                                     double* minHeight,
                                     double* maxHeight) {
    // TBD
}
                                                                    
bool TerrainData::isPositionIndoors(const Coordinates* c) {
    return m_urbanData->isPositionIndoors(c);
}
                                                                    
void TerrainData::print() {
    std::cout << "Terrain Dimensions" << std::endl;
    std::cout << "SW = (" << m_sw.common.c1 << ", " << m_sw.common.c2
              << ")" << std::endl;
    std::cout << "NE = (" << m_ne.common.c1 << ", " << m_ne.common.c2
              << ")" << std::endl;
    std::cout << "Dimensions = (" << m_dimensions.common.c1
              << ", " << m_dimensions.common.c2 << ")" << std::endl;
    std::cout << "Elevation model is " << getElevationModel() << std::endl;
    std::cout << "Urban model is " << getUrbanModel() << std::endl;
}
