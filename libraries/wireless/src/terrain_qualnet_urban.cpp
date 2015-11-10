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
#include <math.h>

#include <vector>
#include <iostream>

#include "api.h"
#include "terrain_qualnet_urban.h"
#include "terrain_qualnet_urban_parser.h"

#define DEBUG   1
#define NODEBUG 0

static void ConvertToGCC(int coordinateSystemType,
                         const Coordinates* c,
                         Coordinates* gccCoord) {
    if (coordinateSystemType == LATLONALT)
    {
        if (c->type != GEOCENTRIC_CARTESIAN)
        {
            COORD_ChangeCoordinateSystem(
                GEODETIC,
                c,
                GEOCENTRIC_CARTESIAN,
                gccCoord);
        }
    }
    else {
        *gccCoord = *c;
    }
}

static void ConvertToGeodetic(int coordinateSystemType,
                              const Coordinates* c,
                              Coordinates* geodeticCoord) {
    if (coordinateSystemType == LATLONALT)
    {
        if (c->type == GEOCENTRIC_CARTESIAN)
        {
            COORD_ChangeCoordinateSystem(
                c,
                GEODETIC,
                geodeticCoord);
        }
    }
    else {
        *geodeticCoord = *c;
    }
}

static void ProjectTo2D(int coordinateSystemType,
                        const Coordinates* c,
                        Coordinates* projectedCoord) {
    // assume c is geodetic
    Coordinates temp;
    if (coordinateSystemType == LATLONALT)
    {
        temp = *c;
        temp.common.c3 = 0;
        COORD_ChangeCoordinateSystem(
            GEODETIC,
            &temp,
            GEOCENTRIC_CARTESIAN,
            projectedCoord);
    }
    else {
        *projectedCoord = *c;
        projectedCoord->common.c3 = 0;
    }
}


void QualNetUrbanPathProperties::sortBuildings() {
    // iterate through buildings.  calculate distance, etc. and put in sorted
    // order by distance from source.
    std::set<BuildingID>::const_iterator thisBuilding;
    std::vector<BuildingData>::iterator  thisBuildingData;
    BuildingData buildingData;

    if (m_alreadySorted) return;
    m_alreadySorted = true;

    if (m_numBuildings == 0) return;

    for (thisBuilding = m_pathData->buildingIDs.begin();
            thisBuilding != m_pathData->buildingIDs.end();
            thisBuilding++)
    {
        double distance1, distance2;

        buildingData.building = *thisBuilding;

        COORD_CalcDistance(CARTESIAN, &m_sourceGCC,
                           &(m_pathData->buildingIntersections[*thisBuilding].point1),
                           &distance1);

        COORD_CalcDistance(CARTESIAN, &m_sourceGCC,
                           &(m_pathData->buildingIntersections[*thisBuilding].point2),
                           &distance2);

        if (distance1 <= distance2) {
            buildingData.distance1 = distance1;
            buildingData.distance2 = distance2;
            buildingData.c1 = m_pathData->buildingIntersections[*thisBuilding].point1;
            buildingData.c2 = m_pathData->buildingIntersections[*thisBuilding].point2;
            buildingData.f1 = m_pathData->buildingFaces[*thisBuilding].f1;
            buildingData.f2 = m_pathData->buildingFaces[*thisBuilding].f2;
        }
        else {
            buildingData.distance1 = distance2;
            buildingData.distance2 = distance1;
            buildingData.c1 = m_pathData->buildingIntersections[*thisBuilding].point2;
            buildingData.c2 = m_pathData->buildingIntersections[*thisBuilding].point1;
            buildingData.f1 = m_pathData->buildingFaces[*thisBuilding].f2;
            buildingData.f2 = m_pathData->buildingFaces[*thisBuilding].f1;
        }

        // insert into building list
        for (thisBuildingData = m_buildingList.begin();
                thisBuildingData != m_buildingList.end();
                thisBuildingData++) {
            if (buildingData < *thisBuildingData) {
                break;
            }
        }
        m_buildingList.insert(thisBuildingData, buildingData);
    }
}

void QualNetUrbanPathProperties::sortFoliage() {
    // iterate through buildings and put into vector.

    std::set<BuildingID>::const_iterator thisFoliage;
    FoliageData foliageData;

    if (m_alreadySortedFoliage)
        return;

    m_alreadySortedFoliage = true;

    if (m_numFoliage == 0) return;

    for (thisFoliage = m_pathData->foliageIDs.begin();
            thisFoliage != m_pathData->foliageIDs.end();
            thisFoliage++)
    {

        foliageData.foliage = *thisFoliage;

        COORD_CalcDistance(CARTESIAN,
                           &(m_pathData->foliageIntersections[*thisFoliage].point1),
                           &(m_pathData->foliageIntersections[*thisFoliage].point2),
                           &foliageData.distanceThrough);

        foliageData.foliatedState = m_urbanData->m_foliatedState;
        foliageData.density = m_urbanData->m_foliage[*thisFoliage].density;
        // insert into foliage list
        m_foliageList.push_back(foliageData);
    }
}

//! Calculates the max, min, and avg building heights.
void QualNetUrbanPathProperties::calculateBuildingHeights() {
    if (m_alreadyCalculatedBuildingHeights) {
        return;
    }
    m_alreadyCalculatedBuildingHeights = true;

    if (m_numBuildings == 0) return;

    double sumHeight = 0.0;
    double maxHeight = 0.0;
    double minHeight = 1000.0; // very tall building
    std::set<BuildingID>::const_iterator thisBuilding;

    for (thisBuilding = m_pathData->buildingIDs.begin();
            thisBuilding != m_pathData->buildingIDs.end();
            thisBuilding++)
    {
        double thisHeight = m_urbanData->m_buildings[*thisBuilding].height;
        sumHeight += thisHeight;
        maxHeight = MAX(maxHeight, thisHeight);
        minHeight = MIN(minHeight, thisHeight);
    }

    m_avgBuildingHeight = (sumHeight / m_numBuildings);
    m_maxRoofHeight     = maxHeight;
    m_minRoofHeight     = minHeight;
}

//! Calculates distance to building, separation, street width
void QualNetUrbanPathProperties::calculateBuildingDistances() {
    if (m_alreadyCalculatedBuildingDistances) {
        return;
    }

    m_alreadyCalculatedBuildingDistances = true;

    // sanity check
    if (m_numBuildings == 0) {
        return;
    }
    if (!m_alreadySorted) {
        sortBuildings();
    }

    unsigned int thisBuilding;
    BuildingData data1;
    BuildingData data2;
    double totalSeparation = 0.0;

    data1 = m_buildingList.front();
    data2 = m_buildingList.back();

    m_srcDistanceToBuilding = data1.distance1;
    m_destDistanceToBuilding = (m_distance - data2.distance2);

    if (m_pathData->numBuildings == 1)
    {
        // Because there's only one building, there's no way to measure
        // distance between buildings, so we estimate by taking average
        // of distance from source to building and building to dest.

        m_avgBuildingSeparation = fabs((m_srcDistanceToBuilding +
                                        m_destDistanceToBuilding) / 2.0);
    }
    else
    {
        for (thisBuilding = 0;
                thisBuilding < (m_buildingList.size() - 1);
                thisBuilding++)
        {
            double tempDistance;
            data1 = m_buildingList[thisBuilding];
            data2 = m_buildingList[thisBuilding+1];

            // separation is the 2nd building's 1st face - the first
            // building's 2nd face.

            tempDistance = fabs(data2.distance1 - data1.distance2);

            if (tempDistance > 0) {
                // the buildings are in a line on the path.
                totalSeparation += tempDistance;
            }
            else {
                // if this is negative, it means the buildings are next to each
                // other rather than in a line along the path.  although the
                // fresnel zone is rarely more than 20m wide, we could still
                // have a situation where two rows of buildings on opposite sides
                // of the road are both in the path.
                // Realistically, the current calculation only works when the path
                // hits buildings broadside. If the path intersects corners of
                // buildings at an angle, the avg separation will be recorded as
                // abnormally large.  Probably should use the region avg instead.
                // TBD, for now, ignoring this case.
            }
        }
        m_avgBuildingSeparation =
            MAX((totalSeparation / (m_buildingList.size() - 1)),
                MIN_BUILDING_SEPARATION);
    }

    m_avgStreetWidth = m_avgBuildingSeparation / 2.0;
}

double QualNetUrbanPathProperties::getSrcDistanceToIntersection() {
    // TBD
    if (!m_alreadyCalculatedSrcDistanceToIntersection) {
        m_alreadyCalculatedSrcDistanceToIntersection = true;
    }
    return m_srcDistanceToIntersection;
}


double QualNetUrbanPathProperties::getDestDistanceToIntersection() {
    // TBD
    if (!m_alreadyCalculatedDestDistanceToIntersection) {
        m_alreadyCalculatedDestDistanceToIntersection = true;
    }
    return m_destDistanceToIntersection;
}


double QualNetUrbanPathProperties::getIntersectionAngle() {
    // TBD
    if (!m_alreadyCalculatedIntersectionAngle) {
        m_alreadyCalculatedIntersectionAngle = true;
    }
    return m_intersectionAngle;
}


//! orientation (used by COST-WI and maybe ITU-R)
double QualNetUrbanPathProperties::getRelativeOrientation() {
    if (!m_alreadyCalculatedRelativeOrientation) {
        m_alreadyCalculatedRelativeOrientation = true;

        BuildingData data;
        FeatureFace  face;

        double lineOfSightNormalX;
        double lineOfSightNormalY;
        double lineOfSightNormalZ;
        Coordinates sourceGeodetic;
        Coordinates destGeodetic;
        Coordinates sourceProjected;
        Coordinates destProjected;

        data = m_buildingList.back(); // this should be closest to receiver.

        // the original code looks at the angle facing node2, presumably
        // the receiver.
        // TBD, we have to choose the first building the node isn't inside
        // i.e. not FACE_RECEIVER or FACE_TRANSMITTER
        face = m_urbanData->m_buildings[data.building].faces[data.f2];

        // take angle between face and line of sight in 2D
        // we remove the z component for the line of sight in CARTESIAN
        // we remove the altitude for the line of sight in LATLONALT
        //
        // the cosine of the normal of the face and the line of sight
        // is the sine of the desired angle
        //
        // we assume the normal of the face is perpendicular to z or altitude

        // TBD, we should replace this with a geometry function
        if (m_coordinateSystemType == LATLONALT)
        {
            ConvertToGeodetic(m_coordinateSystemType,
                              &m_sourceGCC,
                              &sourceGeodetic);
            ConvertToGeodetic(m_coordinateSystemType,
                              &m_destGCC,
                              &destGeodetic);
            ProjectTo2D(m_coordinateSystemType,
                        &sourceGeodetic,
                        &sourceProjected);
            ProjectTo2D(m_coordinateSystemType,
                        &destGeodetic,
                        &destProjected);

            lineOfSightNormalX = destProjected.cartesian.x
                                 - sourceProjected.cartesian.x;
            lineOfSightNormalY = destProjected.cartesian.y
                                 - sourceProjected.cartesian.y;
            lineOfSightNormalZ = destProjected.cartesian.z
                                 - sourceProjected.cartesian.z;
        }
        else
        {
            lineOfSightNormalX = m_dest.cartesian.x - m_source.cartesian.x;
            lineOfSightNormalY = m_dest.cartesian.y - m_source.cartesian.y;
            lineOfSightNormalZ = 0.0;
        }

        double magnitude = sqrt(lineOfSightNormalX * lineOfSightNormalX
                                + lineOfSightNormalY * lineOfSightNormalY
                                + lineOfSightNormalZ * lineOfSightNormalZ);

        lineOfSightNormalX /= magnitude;
        lineOfSightNormalY /= magnitude;
        lineOfSightNormalZ /= magnitude;

        // the dot product of normal vectors is the cosine of the angle
        // between them

        double dotProduct = (face.plane.normalX * lineOfSightNormalX)
                            + (face.plane.normalY * lineOfSightNormalY)
                            + (face.plane.normalZ * lineOfSightNormalZ);

        m_relativeOrientation = (fabs(asin(dotProduct)) / IN_RADIAN);
    }

    return m_relativeOrientation;
}


double QualNetUrbanPathProperties::getAvgFoliageHeight() {

    double sumHeight = 0.0;
    std::set<BuildingID>::const_iterator thisFoliage;

    if (m_numFoliage > 0) {
        for (thisFoliage = m_pathData->foliageIDs.begin();
                thisFoliage != m_pathData->foliageIDs.end();
                thisFoliage++)
        {
            double thisHeight = m_urbanData->m_foliage[*thisFoliage].height;
            sumHeight += thisHeight;
        }
        m_avgFoliageHeight = (sumHeight / m_numFoliage);
    }
    else {
        m_avgFoliageHeight = 0.0;
    }

    return m_avgFoliageHeight;
}


void QualNetUrbanPathProperties::print() {
    int thisBuilding;
    int thisFoliage;

    sortBuildings();
    sortFoliage();
    calculateBuildingHeights();
    calculateBuildingDistances();

    std::cout << "Printing path properties" << std::endl;
    std::cout << "  numBuildings = "               << m_numBuildings << std::endl;
    std::cout << "  avgBuildingSeparation = "      << m_avgBuildingSeparation << std::endl;
    std::cout << "  avgBuildingHeight = "          << m_avgBuildingHeight << std::endl;
    std::cout << "  srcDistanceToIntersection = "  << m_srcDistanceToIntersection << std::endl;
    std::cout << "  destDistanceToIntersection = " << m_destDistanceToIntersection << std::endl;
    std::cout << "  intersectionAngle = "          << m_intersectionAngle << std::endl;
    std::cout << "  avgFoliageHeight = "           << m_avgFoliageHeight << std::endl;
    std::cout << "  maxRoofHeight = "              << m_maxRoofHeight << std::endl;
    std::cout << "  minRoofHeight = "              << m_minRoofHeight << std::endl << std::endl;

    std::cout << std::endl << "Printing buildings" << std::endl;
    for (thisBuilding = 0; thisBuilding < m_numBuildings; thisBuilding++)
    {
        BuildingData data = m_buildingList[thisBuilding];

        std::cout << "  building " << thisBuilding << std::endl;
        std::cout << "  id = " << data.building << std::endl;
        std::cout << "  distance1 = " << data.distance1 << std::endl;
        std::cout << "  distance2 = " << data.distance2 << std::endl;
    }

    std::cout << std::endl << "Printing foliage" << std::endl;
    for (thisFoliage = 0; thisFoliage < m_numFoliage; thisFoliage++)
    {
        FoliageData data = m_foliageList[thisFoliage];

        std::cout << "  foliage " << thisFoliage << std::endl;
        std::cout << "  id = " << data.foliage << std::endl;
        std::cout << "  distance = " << data.distanceThrough << std::endl;
        std::cout << "  density = " << data.density << std::endl;
        std::cout << "  state = " << data.foliatedState << std::endl;
    }
}



QualNetUrbanTerrainData::~QualNetUrbanTerrainData() {
    delete m_buildings;
    delete m_foliage;
    delete m_roadSegments;
    delete m_intersections;
    delete m_parks;
    delete m_stations;
    if (m_roadSegmentVar != NULL) {
        MEM_free(m_roadSegmentVar);
    }
    if (m_parkStationVar != NULL) {
        MEM_free(m_parkStationVar);
    }
}

void QualNetUrbanTerrainData::initialize(
    NodeInput* nodeInput,
    bool       masterProcess)
{
    ParseQualNetUrbanTerrain(m_terrainData,
                             this,
                             nodeInput,
                             masterProcess);
}

void QualNetUrbanTerrainData::finalize()
{
    int i, j;

    //Free Road Segments
    for (i = 0; i < m_numRoadSegments; i++) {
        if (m_roadSegments[i].XML_ID != NULL) {
            MEM_free(m_roadSegments[i].XML_ID);
        }
        if (m_roadSegments[i].streetName != NULL) {
            MEM_free(m_roadSegments[i].streetName);
        }
        MEM_free(m_roadSegments[i].vertices);
    }
    if (m_numRoadSegments > 0)
    {
        MEM_free(m_roadSegments);
    }

    //Free Intersections
    for (i = 0; i < m_numIntersections; i++) {
        if (m_intersections[i].XML_ID != NULL) {
            MEM_free(m_intersections[i].XML_ID);
        }
        MEM_free(m_intersections[i].roadSegments);
        if (m_intersections[i].signalGroups != NULL)
        {
            MEM_free(m_intersections[i].signalGroups);
        }
    }
    if (m_numIntersections > 0)
    {
        MEM_free(m_intersections);
    }

    //Free Parks
    for (i = 0; i < m_numParks; i++) {
        if (m_parks[i].XML_ID != NULL) {
            MEM_free(m_parks[i].XML_ID);
        }
        //        if (m_parks[i].parkName != NULL) {
        //            MEM_free(m_parks[i].parkName);
        //        }
        MEM_free(m_parks[i].vertices);
        MEM_free(m_parks[i].exitIntersections);
    }
    if (m_numParks > 0)
    {
        MEM_free(m_parks);
    }

    //Free Stations
    for (i = 0; i < m_numStations; i++) {
        if (m_stations[i].XML_ID != NULL) {
            MEM_free(m_stations[i].XML_ID);
        }
        //        if (m_stations[i].stationName != NULL) {
        //            MEM_free(m_stations[i].stationName);
        //        }
        MEM_free(m_stations[i].vertices);
        MEM_free(m_stations[i].exitIntersections);
    }
    if (m_numStations > 0)
    {
        MEM_free(m_stations);
    }

    //Free Buildings and their Faces
    for (i = 0; i < m_numBuildings; i++) {
        if (m_buildings[i].XML_ID != NULL) {
            MEM_free(m_buildings[i].XML_ID);
        }
        if (m_buildings[i].featureName != NULL) {
            MEM_free(m_buildings[i].featureName);
        }

        for (j = 0; j < m_buildings[i].num_faces; j++) {
            if (m_buildings[i].faces[j].XML_ID != NULL) {
                MEM_free(m_buildings[i].faces[j].XML_ID);
            }
            MEM_free(m_buildings[i].faces[j].vertices);
        }
        MEM_free(m_buildings[i].faces);

    }
    if (m_numBuildings > 0)
    {
        MEM_free(m_buildings);
    }

    //Free Foliage and their Faces
    for (i = 0; i < m_numFoliage; i++) {
        if (m_foliage[i].XML_ID != NULL) {
            MEM_free(m_foliage[i].XML_ID);
        }
        if (m_foliage[i].featureName != NULL) {
            MEM_free(m_foliage[i].featureName);
        }

        for (j = 0; j < m_foliage[i].num_faces; j++) {
            if (m_foliage[i].faces[j].XML_ID != NULL) {
                MEM_free(m_foliage[i].faces[j].XML_ID);
            }
            MEM_free(m_foliage[i].faces[j].vertices);
        }
        MEM_free(m_foliage[i].faces);

    }
    if (m_numFoliage > 0)
    {
        MEM_free(m_foliage);
    }
}

void QualNetUrbanTerrainData::populateRegionData(TerrainRegion* region,
                                                 TerrainRegion::Mode mode) {

    // TBD, haven't calculated street width.
    // The terrain data does not have information on min/maxElevation,
    // any indoor properties, or foliated state.
    int thisOne;

    double maxRoofHeight     = 0.0;
    double minRoofHeight     = 0.0;
    double totalRoofHeight   = 0.0;
    int    numBuildings      = 0;
    double buildingCoverage  = 0.0;
    int    numFoliage      = 0;
    double totalFoliageCoverage   = 0.0;
    double totalFoliageHeight  = 0.0;
    double totalFoliageDensity = 0.0;

    // find all the buildings in the region
    for (thisOne = 0; thisOne < m_numBuildings; thisOne++) {
        if (!region->intersects(&(m_buildings[thisOne].footprint))) {
            if (NODEBUG) {
                printf("building %s is not in the region\n",
                       m_buildings[thisOne].XML_ID);
            }
            continue;
        }
        if (NODEBUG) {
            printf("building %s is in the region\n", m_buildings[thisOne].XML_ID);
        }

        numBuildings++;
        maxRoofHeight = MAX(maxRoofHeight, m_buildings[thisOne].height);
        minRoofHeight = MIN(maxRoofHeight, m_buildings[thisOne].height);
        totalRoofHeight += m_buildings[thisOne].height;
        buildingCoverage += m_buildings[thisOne].getCoverageArea(
            m_terrainData->getCoordinateSystem());
    }
    region->setNumBuildings(numBuildings);
    region->setMaxRoofHeight(maxRoofHeight);
    if (numBuildings > 0) {
        region->setAvgRoofHeight(totalRoofHeight / (double) numBuildings);
        region->setMinRoofHeight(minRoofHeight);
    }
    region->setBuildingCoverage(buildingCoverage / region->getArea());

    if (mode == TerrainRegion::SIMPLE || mode == TerrainRegion::BUILDINGS) {
        return;
    }

    // find all the foliage in the region
    for (thisOne = 0; thisOne < m_numFoliage; thisOne++) {
        if (region->intersects(&(m_foliage[thisOne].footprint))) {
            continue;
        }

        numFoliage++;
        totalFoliageHeight += m_foliage[thisOne].height;
        totalFoliageDensity += m_foliage[thisOne].density;
        totalFoliageCoverage += m_foliage[thisOne].getCoverageArea(
            m_terrainData->getCoordinateSystem());
    }
    region->setFoliageCoverage(totalFoliageCoverage / region->getArea());
    region->setNumFoliage(numFoliage);
    if (numFoliage > 0) {
        region->setAvgFoliageHeight(totalFoliageHeight / (double) numFoliage);
        region->setAvgFoliageDensity(totalFoliageDensity / (double) numFoliage);
    }
}

std::list<LineSegment>* QualNetUrbanTerrainData::constructPath(
    const  Coordinates* c1,
    const  Coordinates* c2,
    double pathWidth)
{
    // function assumes all coordinates are in Cartesian or GCC

    std::list<LineSegment>* newList = new std::list<LineSegment>;

    LineSegment ls;

    Coordinates source = *c1;
    Coordinates dest   = *c2;

    // Add the original line segment to the path
    ls.point1 = source;
    ls.point2 = dest;
    newList->push_back(ls);

    if (pathWidth == 0.0) {
        // simple path not a fresnel path
        return newList;
    }

    // find unit vectors to parameterize
    // the circles at the ends of the cylinder/fresnel zone

    double tempVector1X = source.cartesian.x - dest.cartesian.x;
    double tempVector1Y = source.cartesian.y - dest.cartesian.y;
    double tempVector1Z = source.cartesian.z - dest.cartesian.z;

    double magnitude = sqrt(tempVector1X * tempVector1X
                            + tempVector1Y * tempVector1Y
                            + tempVector1Z * tempVector1Z);

    tempVector1X /= magnitude;
    tempVector1Y /= magnitude;
    tempVector1Z /= magnitude;

    double unitVector1X;
    double unitVector1Y;
    double unitVector1Z;

    double tempVector2X;
    double tempVector2Y;
    double tempVector2Z;

    double step = 2;

    do
    {
        // random vector that is hopefully not parallel to the line of sight
        tempVector2X = tempVector1X + step;
        tempVector2Y = tempVector1Y + step * step;
        tempVector2Z = tempVector1Z + step * step * step;

        step *= 2;

        unitVector1X = tempVector1Y * tempVector2Z
                       - tempVector1Z * tempVector2Y;
        unitVector1Y = tempVector1Z * tempVector2X
                       - tempVector1X * tempVector2Z;
        unitVector1Z = tempVector1X * tempVector2Y
                       - tempVector1Y * tempVector2X;

        magnitude = sqrt(unitVector1X * unitVector1X
                         + unitVector1Y * unitVector1Y
                         + unitVector1Z * unitVector1Z);
    } while (magnitude < SMALL_NUM);
    // vectors were close to parallel

    unitVector1X /= magnitude;
    unitVector1Y /= magnitude;
    unitVector1Z /= magnitude;

    double unitVector2X = tempVector1Y * unitVector1Z
                          - tempVector1Z * unitVector1Y;
    double unitVector2Y = tempVector1Z * unitVector1X
                          - tempVector1X * unitVector1Z;
    double unitVector2Z = tempVector1X * unitVector1Y
                          - tempVector1Y * unitVector1X;

    magnitude = sqrt(unitVector2X * unitVector2X
                     + unitVector2Y * unitVector2Y
                     + unitVector2Z * unitVector2Z);

    unitVector2X /= magnitude;
    unitVector2Y /= magnitude;
    unitVector2Z /= magnitude;

    double radius = pathWidth / 2;

    bool firstPass = true;

    double angle = 0.0;
    double delta = PI / 4; // use to change number of test points
    double offset = PI;

    // test pairs of points around the circles at the end of the cylinder
    while (angle < 4 * PI)
    {
        if (angle >= 2 * PI && firstPass)
        {
            firstPass = false;
            angle = 2 * PI;
            delta = PI / 2;
            offset = 0.0;
        }

        tempVector1X = radius * (cos(angle) * unitVector1X
                                 + sin(angle) * unitVector2X);
        tempVector1Y = radius * (cos(angle) * unitVector1Y
                                 + sin(angle) * unitVector2Y);
        tempVector1Z = radius * (cos(angle) * unitVector1Z
                                 + sin(angle) * unitVector2Z);

        tempVector2X = radius * (cos(angle + offset) * unitVector1X
                                 + sin(angle + offset) * unitVector2X);
        tempVector2Y = radius * (cos(angle + offset) * unitVector1Y
                                 + sin(angle + offset) * unitVector2Y);
        tempVector2Z = radius * (cos(angle + offset) * unitVector1Z
                                 + sin(angle + offset) * unitVector2Z);


        ls.point1.cartesian.x = source.cartesian.x + tempVector1X;
        ls.point1.cartesian.y = source.cartesian.y + tempVector1Y;
        ls.point1.cartesian.z = source.cartesian.z + tempVector1Z;

        ls.point2.cartesian.x = dest.cartesian.x + tempVector2X;
        ls.point2.cartesian.y = dest.cartesian.y + tempVector2Y;
        ls.point2.cartesian.z = dest.cartesian.z + tempVector2Z;

        newList->push_back(ls);

        angle += delta;
    }

    return newList;
}

TerrainPathData* QualNetUrbanTerrainData::getFeaturesOnPath(
    std::list<LineSegment>* segmentList,
    bool includeFoliage)
{
    TerrainPathData* pathData = new TerrainPathData();

    std::list<LineSegment>::const_iterator iter;

    LineSegment thisSegment;

    int i;

    int numTempFeatures;

    IntersectedPoints ipoints;
    IntersectedFaces  ifaces;

    BuildingID* tempFeatures;
    Coordinates (* tempIntersections)[2];
    FaceIndex (* tempFaces)[2];

    for (iter = segmentList->begin(); iter != segmentList->end(); iter++)
    {
        thisSegment = *iter;

        // return the buildings (and associated data) for buildings that intersect
        // this line segment
        returnIntersectionBuildings(m_buildings,
                                    m_numBuildings,
                                    &(thisSegment.point1),
                                    &(thisSegment.point2),
                                    &numTempFeatures,
                                    &tempFeatures,
                                    &tempIntersections,
                                    &tempFaces);

        if (NODEBUG) {
            printf("for ls from (%f,%f,%f) to (%f,%f,%f), there are %d buildings\n",
                   thisSegment.point1.common.c1,
                   thisSegment.point1.common.c2,
                   thisSegment.point1.common.c3,
                   thisSegment.point2.common.c1,
                   thisSegment.point2.common.c2,
                   thisSegment.point2.common.c3,
                   numTempFeatures);
        }

        for (i = 0; i < numTempFeatures; i++)
        {
            BuildingID thisBuilding = tempFeatures[i];

            if (NODEBUG) {
                printf("\t%s\n", m_buildings[thisBuilding].XML_ID);
            }

            // This code skips buildings that contain one of the nodes.
            // For buildings, this may be OK, because this segment should be indoor
            // propagation.
            if (tempFaces[i][0] == FACE_RECEIVER
                    || tempFaces[i][0] == FACE_TRANSMITTER
                    || tempFaces[i][1] == FACE_RECEIVER
                    || tempFaces[i][1] == FACE_TRANSMITTER)
            {
                continue;
            }

            // for each of the new buildings, check to see if they're already in the list
            if (pathData->buildingIDs.count(thisBuilding) == 1)
            {
                // it's already in the list, maybe update values
            }
            else { // add new
                pathData->buildingIDs.insert(thisBuilding);
                ipoints.point1 = tempIntersections[i][0];
                ipoints.point2 = tempIntersections[i][1];
                ifaces.f1  = tempFaces[i][0];
                ifaces.f2  = tempFaces[i][1];
                pathData->buildingIntersections[thisBuilding] = ipoints;
                pathData->buildingFaces[thisBuilding]         = ifaces;
                pathData->numBuildings++;
            }
        }

        MEM_free(tempFeatures);
        MEM_free(tempIntersections);
        MEM_free(tempFaces);

        if (includeFoliage) {
            // return the foliage (and associated data) for foliage that
            // intersects this line segment
            returnIntersectionBuildings(m_foliage,
                                        m_numFoliage,
                                        &(thisSegment.point1),
                                        &(thisSegment.point2),
                                        &numTempFeatures,
                                        &tempFeatures,
                                        &tempIntersections,
                                        &tempFaces);

            for (i = 0; i < numTempFeatures; i++)
            {
                // For buildings, we skipped this because the indoor segment
                // has to be handled differently, but for foliage, we want
                // to consider the case where the node is inside the foliage.
                // TBD, this will mean removing this and changing the code
                // where we calculate distances through foliage to consider
                // this case. Possibly we should change this for OPAR too,
                // or add a parameter.
                if (tempFaces[i][0] == FACE_RECEIVER
                    || tempFaces[i][0] == FACE_TRANSMITTER
                    || tempFaces[i][1] == FACE_RECEIVER
                    || tempFaces[i][1] == FACE_TRANSMITTER)
                {
                    continue;
                }

                BuildingID thisFoliage = tempFeatures[i];

                // for each of the new foliages, check to see if they're
                // already in the list
                if (pathData->foliageIDs.count(thisFoliage) == 1)
                {
                    // it's already in the list, maybe update values
                }
                else { // add new
                    pathData->foliageIDs.insert(thisFoliage);
                    ipoints.point1 = tempIntersections[i][0];
                    ipoints.point2 = tempIntersections[i][1];
                    ifaces.f1  = tempFaces[i][0];
                    ifaces.f2  = tempFaces[i][1];
                    pathData->foliageIntersections[thisFoliage] = ipoints;
                    pathData->foliageFaces[thisFoliage]         = ifaces;
                    pathData->numFoliage++;
                }
            }

            MEM_free(tempFeatures);
            MEM_free(tempIntersections);
            MEM_free(tempFaces);
        }
    }

    return pathData;
}

UrbanPathProperties* QualNetUrbanTerrainData::getUrbanPathProperties(
    const  Coordinates* c1,
    const  Coordinates* c2,
    double pathWidth,
    bool   includeFoliage)
{
    std::list<LineSegment>*     constructedPath;
    TerrainPathData*                   pathFeatures;
    QualNetUrbanPathProperties* pathProps;

    Coordinates sourceGCC;
    Coordinates destGCC;

    convertToGCC(c1, &sourceGCC);
    convertToGCC(c2, &destGCC);

    pathProps = new QualNetUrbanPathProperties(
                    this, c1, c2,
                    m_terrainData->getCoordinateSystem(),
                    &sourceGCC,
                    &destGCC);

    if ((m_numBuildings == 0) && (m_numFoliage == 0))
    {
        // everything will be 0.
        return pathProps;
    }

    constructedPath = constructPath(&sourceGCC, &destGCC, pathWidth);
    if (NODEBUG) {
        printf("constructed path of %"TYPES_SIZEOFMFT"u elements\n", constructedPath->size());
    }
    pathFeatures    = getFeaturesOnPath(constructedPath,
                                        includeFoliage);

    if (NODEBUG)
    {
        printf("numbuildings on path = %d\n", pathFeatures->numBuildings);
    }
    pathProps->setNumBuildings(pathFeatures->numBuildings);
    pathProps->setNumFoliage(pathFeatures->numFoliage);
    pathProps->setPathData(pathFeatures);

    delete constructedPath;

    return pathProps;
}

//! This function tells me whether the line between source/dest passes through
//  the obstruction, and where.
bool QualNetUrbanTerrainData::calculateOverlap(
    int coordinateSystemType,
    const Coordinates* source,
    const Coordinates* dest,
    Building* obstruction,
    Coordinates* const intersection1,
    Coordinates* const intersection2,
    FaceIndex* const face1,
    FaceIndex* const face2)
{
    // source and dest are in GCC by this point

    bool sourceIsInside = false;
    bool destIsInside   = false;
    int i;
    int intersectionCount;

    Cube* cube = &(obstruction->boundingCube);
    
    if (NODEBUG) {
        printf("checking building %s with ", obstruction->XML_ID);
        cube->print();
    }

    if ((source->cartesian.x == dest->cartesian.x)
        && (source->cartesian.y == dest->cartesian.y)
        && (source->cartesian.z == dest->cartesian.z))
    {
        return false;
    }

    // TBD, this following check will only work for rectangular buildings
    // We need a geometry function for finding whether a point is inside
    // an arbitrary shape building.
    if ((source->cartesian.x >= cube->x()) &&
        (source->cartesian.y >= cube->y()) &&
        (source->cartesian.z >= cube->z()) &&
        (source->cartesian.x <= cube->X()) &&
        (source->cartesian.y <= cube->Y()) &&
        (source->cartesian.z <= cube->Z()))
    {
        if (NODEBUG) {
            printf("source is within bounds\n");
        }
        sourceIsInside = true;
    }
    if ((dest->cartesian.x >= cube->x()) &&
        (dest->cartesian.y >= cube->y()) &&
        (dest->cartesian.z >= cube->z()) &&
        (dest->cartesian.x <= cube->X()) &&
        (dest->cartesian.y <= cube->Y()) &&
        (dest->cartesian.z <= cube->Z()))
    {
        if (NODEBUG) {
            printf("dest is within bounds\n");
        }
        destIsInside = true;
    }

    if (destIsInside && sourceIsInside) {
        // since both nodes are inside, there's no intersections, though the
        // distance through will of course be the distance.  possibly we should
        // return the two node positions, with FACE_RECEIVER and FACE_TRANSMITTER
        return false;
    }

    // test(s) may or may not improve performance
    // depending on hit rate, hit cost, miss cost
    if (!destIsInside &&
        !sourceIsInside &&
        !passThroughTest(source,
                         dest,
                         &(obstruction->boundingCube)))
    {
        return false;
    }
    if (NODEBUG) {
        printf("does pass through building %s\n", obstruction->XML_ID);
    }

    intersectionCount = 0;

    for (i = 0; i < obstruction->num_faces; i++)
    {
        if (calculateIntersectionSegmentPlane(
                coordinateSystemType,
                source,
                dest,
                &(obstruction->faces[i]),
                intersection1))
        {
            if (NODEBUG) {
                printf("face %d is found to intersect at (%f,%f,%f)\n", i,
                       intersection1->cartesian.x,
                       intersection1->cartesian.y,
                       intersection1->cartesian.z);
                obstruction->faces[i].print();
            }
                       
            intersectionCount++;
            (*face1) = i;
            break;
        }
    }

    i++;
    for (; i < obstruction->num_faces; i++)
    {
        if (calculateIntersectionSegmentPlane(
                coordinateSystemType,
                source,
                dest,
                &(obstruction->faces[i]),
                intersection2))
        {
            if (NODEBUG) {
                printf("face %d is found to intersect at (%f,%f,%f)\n", i,
                       intersection2->cartesian.x,
                       intersection2->cartesian.y,
                       intersection2->cartesian.z);
                obstruction->faces[i].print();
            }
            // TBD, not sure about this
            // if intersection is on a border of multiple faces
            // it may be repeated
            if ((fabs(intersection1->cartesian.x - intersection2->cartesian.x)
                 < SMALL_NUM)
                && (fabs(intersection1->cartesian.y - intersection2->cartesian.y)
                    < SMALL_NUM)
                && (fabs(intersection1->cartesian.z - intersection2->cartesian.z)
                    < SMALL_NUM))
            {
                continue;
            }

            intersectionCount++;
            (*face2) = i;
            break;
        }
    }

    if (NODEBUG) {
        printf("source/dest inside = %d/%d\n", sourceIsInside, destIsInside);
    }
    if (intersectionCount == 2)
    {
        return true;
    }
    else if (intersectionCount == 1)
    {
        if (sourceIsInside)
        {
            (*face2) = FACE_TRANSMITTER;
            memcpy(intersection2, source, sizeof(Coordinates));
            return true;
        }
        else if (destIsInside)
        {
            (*face2) = FACE_RECEIVER;
            memcpy(intersection2, dest, sizeof(Coordinates));
            return true;
        }
    }

    return false;
}


//! Returns true if the source/dest line intersects the face, and where.
bool QualNetUrbanTerrainData::calculateIntersectionSegmentPlane(
    int coordinateSystemType,
    const Coordinates* source,
    const Coordinates* dest,
    const FeatureFace* face,
    Coordinates* const intersection)
{
    // assume source/dest, all points in face are in UNREFERENCED_CARTESIAN
    // or GEOCENTRIC_CARTESIAN, depending on the coordinate system

    int    i;
    double mu;
    double dot;
    double tempVector1X;
    double tempVector1Y;
    double tempVector1Z;
    double tempVector2X;
    double tempVector2Y;
    double tempVector2Z;

    // calculate intersection of line with plane

    double denom =
        face->plane.normalX * (dest->cartesian.x - source->cartesian.x)
        + face->plane.normalY * (dest->cartesian.y - source->cartesian.y)
        + face->plane.normalZ * (dest->cartesian.z - source->cartesian.z);
    if (denom == 0)
    {
        // Line and plane don't intersect
        return false;
    }
    mu = - (face->plane.d
            + face->plane.normalX * source->cartesian.x
            + face->plane.normalY * source->cartesian.y
            + face->plane.normalZ * source->cartesian.z)
         / denom;
    if (mu <= 0 || mu >= 1)
    {
        // Intersection not along line segment
        // for mu == 0 or 1
        // treat source/dest as inside when on surface, so no intersection
        return false;
    }
    intersection->cartesian.x = source->cartesian.x
        + mu * (dest->cartesian.x - source->cartesian.x);
    intersection->cartesian.y = source->cartesian.y
        + mu * (dest->cartesian.y - source->cartesian.y);
    intersection->cartesian.z = source->cartesian.z
        + mu * (dest->cartesian.z - source->cartesian.z);
    if (coordinateSystemType == LATLONALT)
    {
        intersection->type = GEOCENTRIC_CARTESIAN;
    }
    else
    {
        intersection->type = UNREFERENCED_CARTESIAN;
    }

    // check within face on plane

    for (i = 0; i < face->num_vertices; i++)
    {
        // vertex i - intersection
        tempVector1X = face->vertices[i].cartesian.x
                       - intersection->cartesian.x;
        tempVector1Y = face->vertices[i].cartesian.y
                       - intersection->cartesian.y;
        tempVector1Z = face->vertices[i].cartesian.z
                       - intersection->cartesian.z;

        // vertex i+1 - intersection
        tempVector2X =
            face->vertices[(i + 1) % face->num_vertices].cartesian.x
            - intersection->cartesian.x;
        tempVector2Y =
            face->vertices[(i + 1) % face->num_vertices].cartesian.y
            - intersection->cartesian.y;
        tempVector2Z =
            face->vertices[(i + 1) % face->num_vertices].cartesian.z
            - intersection->cartesian.z;

        // dot(normal of plane, cross (vi - inter, vi+1 - inter))
        dot = (tempVector1Y * tempVector2Z - tempVector1Z * tempVector2Y)
              * face->plane.normalX
              + (tempVector1Z * tempVector2X - tempVector1X * tempVector2Z)
              * face->plane.normalY
              + (tempVector1X * tempVector2Y - tempVector1Y * tempVector2X)
              * face->plane.normalZ;

        if (dot == 0)
        {
            // on border of face, including possibly on a vertex
            break;
        }

        if (dot < 0)
        {
            // cross product and normal of plane should be in same direction
            // dot product should be positive
            return false;
        }
    }

    return true;
}

//! Function tells us whether the source/dest line passes through the
//  bounding cube defined in properties.
bool QualNetUrbanTerrainData::passThroughTest(
    const Coordinates* source,
    const Coordinates* dest,
    Cube* cube)
{
    // Heuristic function
    // return: false indicates that overlap is not possible
    // return: true indicates that overlap is possible
    //         line goes through box defined by max and min x,y,z's
    CoordinateType tempX;
    CoordinateType tempY;
    CoordinateType tempZ;

    if (((cube->X() <= source->cartesian.x) &&
         (cube->X() <= dest->cartesian.x)) ||
        ((cube->x() >= source->cartesian.x) &&
         (cube->x() >= dest->cartesian.x)) ||
        ((cube->Y() <= source->cartesian.y) &&
         (cube->Y() <= dest->cartesian.y)) ||
        ((cube->y() >= source->cartesian.y) &&
         (cube->y() >= dest->cartesian.y)) ||
        ((cube->Z() <= source->cartesian.z) &&
         (cube->Z() <= dest->cartesian.z)) ||
        ((cube->z() >= source->cartesian.z) &&
         (cube->z() >= dest->cartesian.z)))
    {
        return false;
    }

    if (((dest->cartesian.x < cube->x()) &&
         (cube->x() < source->cartesian.x)) ||
        ((source->cartesian.x < cube->x()) &&
         (cube->x() < dest->cartesian.x)))
    {
        // new point at x
        tempY = source->cartesian.y
            + (dest->cartesian.y - source->cartesian.y)
            * (cube->x() - source->cartesian.x)
            / (dest->cartesian.x - source->cartesian.x);
        tempZ = source->cartesian.z
            + (dest->cartesian.z - source->cartesian.z)
            * (cube->x() - source->cartesian.x)
            / (dest->cartesian.x - source->cartesian.x);

        if ((cube->y() <= tempY) && (tempY <= cube->Y()) &&
            (cube->z() <= tempZ) && (tempZ <= cube->Z()))
        {
            return true;
        }
    }

    if (((dest->cartesian.x < cube->X()) &&
         (cube->X() < source->cartesian.x)) ||
        ((source->cartesian.x < cube->X()) &&
         (cube->X() < dest->cartesian.x)))
    {
        // new point at X
        tempY = source->cartesian.y
            + (dest->cartesian.y - source->cartesian.y)
            * (cube->X() - source->cartesian.x)
            / (dest->cartesian.x - source->cartesian.x);
        tempZ = source->cartesian.z
            + (dest->cartesian.z - source->cartesian.z)
            * (cube->X() - source->cartesian.x)
            / (dest->cartesian.x - source->cartesian.x);

        if ((cube->y() <= tempY) && (tempY <= cube->Y()) &&
            (cube->z() <= tempZ) && (tempZ <= cube->Z()))
        {
            return true;
        }
    }

    if (((dest->cartesian.y < cube->y())
         && (cube->y() < source->cartesian.y))
        ||
        ((source->cartesian.y < cube->y())
         && (cube->y() < dest->cartesian.y)))
    {
        // new point at y
        tempX = source->cartesian.x
            + (dest->cartesian.x - source->cartesian.x)
            * (cube->y() - source->cartesian.y)
            / (dest->cartesian.y - source->cartesian.y);
        tempZ = source->cartesian.z
            + (dest->cartesian.z - source->cartesian.z)
            * (cube->y() - source->cartesian.y)
            / (dest->cartesian.y - source->cartesian.y);

        if ((cube->x() <= tempX) && (tempX <= cube->X()) &&
            (cube->z() <= tempZ) && (tempZ <= cube->Z()))
        {
            return true;
        }
    }

    if (((dest->cartesian.y < cube->Y()) &&
         (cube->Y() < source->cartesian.y)) ||
        ((source->cartesian.y < cube->Y()) &&
         (cube->Y() < dest->cartesian.y)))
    {
        // new point at Y
        tempX = source->cartesian.x
            + (dest->cartesian.x - source->cartesian.x)
            * (cube->Y() - source->cartesian.y)
            / (dest->cartesian.y - source->cartesian.y);
        tempZ = source->cartesian.z
            + (dest->cartesian.z - source->cartesian.z)
            * (cube->Y() - source->cartesian.y)
            / (dest->cartesian.y - source->cartesian.y);

        if ((cube->x() <= tempX) && (tempX <= cube->X())
            && (cube->z() <= tempZ) && (tempZ <= cube->Z()))
        {
            return true;
        }
    }

    if (((dest->cartesian.z < cube->z()) &&
         (cube->z() < source->cartesian.z)) ||
        ((source->cartesian.z < cube->z()) &&
         (cube->z() < dest->cartesian.z)))
    {
        // new point at z
        tempX = source->cartesian.x
            + (dest->cartesian.x - source->cartesian.x)
            * (cube->z() - source->cartesian.z)
            / (dest->cartesian.z - source->cartesian.z);
        tempY = source->cartesian.y
            + (dest->cartesian.y - source->cartesian.y)
            * (cube->z() - source->cartesian.z)
            / (dest->cartesian.z - source->cartesian.z);

        if ((cube->x() <= tempX) && (tempX <= cube->X()) &&
            (cube->y() <= tempY) && (tempY <= cube->Y()))
        {
            return true;
        }
    }

    if (((dest->cartesian.z < cube->Z()) &&
         (cube->Z() < source->cartesian.z)) ||
        ((source->cartesian.z < cube->Z()) &&
         (cube->Z() < dest->cartesian.z)))
    {
        // new point at Z
        tempX = source->cartesian.x
            + (dest->cartesian.x - source->cartesian.x)
            * (cube->Z() - source->cartesian.z)
            / (dest->cartesian.z - source->cartesian.z);
        tempY = source->cartesian.y
            + (dest->cartesian.y - source->cartesian.y)
            * (cube->Z() - source->cartesian.z)
            / (dest->cartesian.z - source->cartesian.z);

        if ((cube->x() <= tempX) && (tempX <= cube->X()) &&
            (cube->y() <= tempY) && (tempY <= cube->Y()))
        {
            return true;
        }
    }

    return false;
}


void QualNetUrbanTerrainData::createBoundingCube(Building* building)
{
    FeatureFace* face;
    int i;
    int j;

    building->boundingCube.lower = building->faces[0].vertices[0];
    building->boundingCube.upper = building->faces[0].vertices[0];

    for (i = 0; i < building->num_faces; i++)
    {
        face = &building->faces[i];
        face->boundingCube.lower = face->vertices[0];
        face->boundingCube.upper = face->vertices[0];

        for (j = 1; j < face->num_vertices; j++)
        {
            // convert each face to cartesian, or assume cartesian

            if (face->vertices[j].cartesian.x < face->boundingCube.x())
            {
                face->boundingCube.x() = face->vertices[j].cartesian.x;
            }
            else if (face->vertices[j].cartesian.x > face->boundingCube.X())
            {
                face->boundingCube.X() = face->vertices[j].cartesian.x;
            }

            if (face->vertices[j].cartesian.y < face->boundingCube.y())
            {
                face->boundingCube.y() = face->vertices[j].cartesian.y;
            }
            else if (face->vertices[j].cartesian.y > face->boundingCube.Y())
            {
                face->boundingCube.Y() = face->vertices[j].cartesian.y;
            }

            if (face->vertices[j].cartesian.z < face->boundingCube.z())
            {
                face->boundingCube.z() = face->vertices[j].cartesian.z;
            }
            else if (face->vertices[j].cartesian.z > face->boundingCube.Z())
            {
                face->boundingCube.Z() = face->vertices[j].cartesian.z;
            }
        }

        if (face->boundingCube.x() < building->boundingCube.x())
        {
            building->boundingCube.x() = face->boundingCube.x();
        }
        if (face->boundingCube.X() > building->boundingCube.X())
        {
            building->boundingCube.X() = face->boundingCube.X();
        }
        if (face->boundingCube.y() < building->boundingCube.y())
        {
            building->boundingCube.y() = face->boundingCube.y();
        }
        if (face->boundingCube.Y() > building->boundingCube.Y())
        {
            building->boundingCube.Y() = face->boundingCube.Y();
        }
        if (face->boundingCube.z() < building->boundingCube.z())
        {
            building->boundingCube.z() = face->boundingCube.z();
        }
        if (face->boundingCube.Z() > building->boundingCube.Z())
        {
            building->boundingCube.Z() = face->boundingCube.Z();
        }
    }

    // setting footprint
    if (m_terrainData->getCoordinateSystem() == CARTESIAN
#ifdef MGRS_ADDON
        || m_terrainData->getCoordinateSystem() == MGRS
#endif // MGRS_ADDON
        )
    {
        building->footprint.setMaxX(building->boundingCube.X());
        building->footprint.setMinX(building->boundingCube.x());
        building->footprint.setMaxY(building->boundingCube.Y());
        building->footprint.setMinY(building->boundingCube.y());
    }
    else // LATLONALT
    {
        // Ignore case where building straddles the dateline, for now.
        // I mean, it's mostly ocean, right?
        for (i = 0; i < building->num_faces; i++)
        {
            face = &building->faces[i];
            Coordinates lower;
            Coordinates upper;
            COORD_ChangeCoordinateSystem(&(face->boundingCube.lower),
                                         GEODETIC,
                                         &lower);
            COORD_ChangeCoordinateSystem(&(face->boundingCube.upper),
                                         GEODETIC,
                                         &upper);

            if (i == 0) {
                building->footprint.lower = lower;
                building->footprint.upper = upper;
            }
            else {
                if (lower.latlonalt.latitude < building->footprint.lower.latlonalt.latitude)
                {
                    building->footprint.lower.latlonalt.latitude = lower.latlonalt.latitude;
                }
                if (lower.latlonalt.longitude < building->footprint.lower.latlonalt.longitude)
                {
                    building->footprint.lower.latlonalt.longitude = lower.latlonalt.longitude;
                }
                if (upper.latlonalt.latitude > building->footprint.upper.latlonalt.latitude)
                {
                    building->footprint.upper.latlonalt.latitude = upper.latlonalt.latitude;
                }
                if (upper.latlonalt.longitude > building->footprint.upper.latlonalt.longitude)
                {
                    building->footprint.upper.latlonalt.longitude = upper.latlonalt.longitude;
                }
            }
        }
    }
    if (NODEBUG) {
        printf("building %s footprint\n", building->XML_ID);
        building->footprint.print();
    }
}

void QualNetUrbanTerrainData::createPlaneParameters(Building* building)
{
    int i;

    Coordinates* pointA;
    Coordinates* pointB;
    Coordinates* pointC;

    double tempVector1X;
    double tempVector1Y;
    double tempVector1Z;
    double tempVector2X;
    double tempVector2Y;
    double tempVector2Z;


    for (i = 0; i < building->num_faces; i++)
    {
        // calculate plane for face based on first 3 vertices
        pointA = &(building->faces[i].vertices[0]);
        pointB = &(building->faces[i].vertices[1]);
        pointC = &(building->faces[i].vertices[2]);

        // Pb - Pa
        tempVector1X = pointB->cartesian.x - pointA->cartesian.x;
        tempVector1Y = pointB->cartesian.y - pointA->cartesian.y;
        tempVector1Z = pointB->cartesian.z - pointA->cartesian.z;
        // Pc - Pa
        tempVector2X = pointC->cartesian.x - pointA->cartesian.x;
        tempVector2Y = pointC->cartesian.y - pointA->cartesian.y;
        tempVector2Z = pointC->cartesian.z - pointA->cartesian.z;
        // normal of plane = (Pb - Pa) cross (Pc - Pa)
        building->faces[i].plane.normalX = tempVector1Y * tempVector2Z
                                           - tempVector1Z * tempVector2Y;
        building->faces[i].plane.normalY = tempVector1Z * tempVector2X
                                           - tempVector1X * tempVector2Z;
        building->faces[i].plane.normalZ = tempVector1X * tempVector2Y
                                           - tempVector1Y * tempVector2X;

        double magnitude;
        magnitude= sqrt(building->faces[i].plane.normalX
                        * building->faces[i].plane.normalX
                        + building->faces[i].plane.normalY
                        * building->faces[i].plane.normalY
                        + building->faces[i].plane.normalZ
                        * building->faces[i].plane.normalZ);
        building->faces[i].plane.normalX /= magnitude;
        building->faces[i].plane.normalY /= magnitude;
        building->faces[i].plane.normalZ /= magnitude;

        building->faces[i].plane.d =
            - building->faces[i].plane.normalX * pointA->cartesian.x
            - building->faces[i].plane.normalY * pointA->cartesian.y
            - building->faces[i].plane.normalZ * pointA->cartesian.z;
    }
}

void QualNetUrbanTerrainData::calculateIntersection3Planes(
    const FeatureFace* face1,
    const FeatureFace* face2,
    const FeatureFace* face3,
    Coordinates* const intersection)
{

    // point = -d1*cross(n2,n3)-d2*cross(n3,n1)-d3*cross(n1,n2)
    //          /dot(n1,cross(n2,n3))

    double cross23X = face2->plane.normalY * face3->plane.normalZ
                      - face2->plane.normalZ * face3->plane.normalY;
    double cross23Y = face2->plane.normalZ * face3->plane.normalX
                      - face2->plane.normalX * face3->plane.normalZ;
    double cross23Z = face2->plane.normalX * face3->plane.normalY
                      - face2->plane.normalY * face3->plane.normalX;

    double denom = face1->plane.normalX * cross23X
                   + face1->plane.normalY * cross23Y
                   + face1->plane.normalZ * cross23Z;

    if (fabs(denom) < SMALL_NUM)
    {
        ERROR_ReportError("calculateIntersection3Planes: "
                          "Requested intersection of three planes that do "
                          "not have a unique point intersection.\n");
    }

    intersection->cartesian.x =
        (- face1->plane.d * cross23X
         - face2->plane.d * (face3->plane.normalY * face1->plane.normalZ
                             - face3->plane.normalZ * face1->plane.normalY)
         - face3->plane.d * (face1->plane.normalY * face2->plane.normalZ
                             - face1->plane.normalZ * face2->plane.normalY))
        / denom;

    intersection->cartesian.y =
        (- face1->plane.d * cross23Y
         - face2->plane.d * (face3->plane.normalZ * face1->plane.normalX
                             - face3->plane.normalX * face1->plane.normalZ)
         - face3->plane.d * (face1->plane.normalZ * face2->plane.normalX
                             - face1->plane.normalX * face2->plane.normalZ))
        / denom;

    intersection->cartesian.z =
        (- face1->plane.d * cross23Z
         - face2->plane.d * (face3->plane.normalX * face1->plane.normalY
                             - face3->plane.normalY * face1->plane.normalX)
         - face3->plane.d * (face1->plane.normalX * face2->plane.normalY
                             - face1->plane.normalY * face2->plane.normalX))
        / denom;
}


double QualNetUrbanTerrainData::cosineOfVectors(
    int coordinateSystemType,
    const Coordinates* source,
    const Coordinates* dest1,
    const Coordinates* dest2)
{
    Coordinates save1;
    Coordinates save2;
    Coordinates save3;

    Coordinates vector1;
    Coordinates vector2;

    if (coordinateSystemType == LATLONALT)
    {
        if (source->type != GEOCENTRIC_CARTESIAN)
        {
            COORD_ChangeCoordinateSystem(
                GEODETIC,
                source,
                GEOCENTRIC_CARTESIAN,
                &save1);
            source = &save1;
        }
        if (dest1->type != GEOCENTRIC_CARTESIAN)
        {
            COORD_ChangeCoordinateSystem(
                GEODETIC,
                dest1,
                GEOCENTRIC_CARTESIAN,
                &save2);
            dest1 = &save2;
        }
        if (dest2->type != GEOCENTRIC_CARTESIAN)
        {
            COORD_ChangeCoordinateSystem(
                GEODETIC,
                dest2,
                GEOCENTRIC_CARTESIAN,
                &save3);
            dest2 = &save3;
        }
    }

    vector1.cartesian.x = dest1->cartesian.x - source->cartesian.x;
    vector1.cartesian.y = dest1->cartesian.y - source->cartesian.y;
    vector1.cartesian.z = dest1->cartesian.z - source->cartesian.z;

    vector2.cartesian.x = dest2->cartesian.x - source->cartesian.x;
    vector2.cartesian.y = dest2->cartesian.y - source->cartesian.y;
    vector2.cartesian.z = dest2->cartesian.z - source->cartesian.z;

    double magnitude1 = sqrt(vector1.cartesian.x * vector1.cartesian.x
                             + vector1.cartesian.y * vector1.cartesian.y
                             + vector1.cartesian.z * vector1.cartesian.z);
    double magnitude2 = sqrt(vector2.cartesian.x * vector2.cartesian.x
                             + vector2.cartesian.y * vector2.cartesian.y
                             + vector2.cartesian.z * vector2.cartesian.z);

    if ((magnitude1 < SMALL_NUM) && (magnitude2 < SMALL_NUM))
    {
#if 0
        ERROR_ReportError("cosineOfVectors: "
                          "First, second, and third coordinates are at the "
                          "same location.\n");
#endif
        return COSINE_ERROR_ALL;
    }

    if (magnitude1 < SMALL_NUM)
    {
#if 0
        ERROR_ReportError("cosineOfVectors: "
                          "First and second coordinates are at the same "
                          "location.\n");
#endif
        return COSINE_ERROR_FIRST_SECOND;
    }

    if (magnitude2 < SMALL_NUM)
    {
#if 0
        ERROR_ReportError("cosineOfVectors: "
                          "First and third coordinates are at the same "
                          "location.\n");
#endif
        return COSINE_ERROR_FIRST_THIRD;
    }

    return (vector1.cartesian.x * vector2.cartesian.x
            + vector1.cartesian.y * vector2.cartesian.y
            + vector1.cartesian.z * vector2.cartesian.z)
           / (magnitude1 * magnitude2);
}

double QualNetUrbanTerrainData::calcDistanceOverTerrain(
    const Coordinates* position1,
    const Coordinates* position2,
    double elevationSamplingDistance)
{
    double distance;

    COORD_CalcDistance(
        LATLONALT,
        position1,
        position2,
        &distance);

    if ((m_terrainData->getCoordinateSystem() == LATLONALT)
            && (m_terrainData->hasElevationData()))
    {
        int numSamples = (int) ceil(distance / elevationSamplingDistance);
        // could also set a maximum; see getelevationarray

        double dLatitude =
            (position2->latlonalt.latitude - position1->latlonalt.latitude)
            / (double) numSamples;
        double dLongitude =
            (position2->latlonalt.longitude - position1->latlonalt.longitude)
            / (double) numSamples;

        Coordinates position;
        //
        // This if statement is added to return the same result
        // for the given two end-points regardless of the direction
        //
        if (position1->latlonalt.latitude >= position2->latlonalt.latitude)
        {
            position.latlonalt.latitude = position1->latlonalt.latitude;
            position.latlonalt.longitude = position1->latlonalt.longitude;
        }
        else
        {
            position.latlonalt.latitude = position2->latlonalt.latitude;
            position.latlonalt.longitude = position2->latlonalt.longitude;
            dLatitude = -dLatitude;
            dLongitude = -dLongitude;
        }

        distance = 0;
        TERRAIN_SetToGroundLevel(m_terrainData, &position);
        double stepDistance;
        Coordinates oldPosition;
        int i;
        for (i = 0; i < numSamples; i++)
        {
            oldPosition.latlonalt.latitude = position.latlonalt.latitude;
            oldPosition.latlonalt.longitude = position.latlonalt.longitude;
            oldPosition.latlonalt.altitude = position.latlonalt.altitude;

            position.latlonalt.latitude += dLatitude;
            position.latlonalt.longitude += dLongitude;

            TERRAIN_SetToGroundLevel(m_terrainData, &position);
            COORD_CalcDistance(
                LATLONALT,
                &position,
                &oldPosition,
                &stepDistance);

            distance += stepDistance;
        }
    }
    else
    {
        ERROR_ReportWarning(
            "calcDistanceOverTerrain: "
            "Coordinate system must be LATLONALT "
            "and there must be terrain data. "
            "Returned direct distance.\n");
    }
    return distance;
}

void QualNetUrbanTerrainData::roadGiveIntermediatePosition(
    Coordinates* const position,
    RoadSegmentID ID,
    IntersectionID startIntersection,
    double traveling_distance)
{
    double accumulated_distance = 0;
    RoadSegment* rs = getRoadSegment(ID);
    double distance = 0;
    int i;

    if (traveling_distance >= rs->length)
    {
        ERROR_ReportError("ENVIRONMENT: tried to travel farther down "
                          "road than length of road\n");
    }

    if (rs->firstIntersection == startIntersection)
    {
        for (i = 0; i <= rs->num_vertices - 2; i++)
        {
            COORD_CalcDistance(m_terrainData->getCoordinateSystem(),
                               &(rs->vertices[i]),
                               &(rs->vertices[i + 1]),
                               &distance);
            if (accumulated_distance + distance <= traveling_distance)
            {
                accumulated_distance += distance;
            }
            else
            {
                break;
            }
        }

        if (i == rs->num_vertices -  1)
        {
            ERROR_ReportError("ENVIRONMENT: tried to travel farther down "
                              "road than length of road\n");
        }

        double fraction
        = (traveling_distance - accumulated_distance) / distance;

        position->common.c1 =
            rs->vertices[i].common.c1
            + (rs->vertices[i + 1].common.c1 - rs->vertices[i].common.c1)
            * fraction;

        position->common.c2 =
            rs->vertices[i].common.c2
            + (rs->vertices[i + 1].common.c2 - rs->vertices[i].common.c2)
            * fraction;

        position->common.c3 =
            rs->vertices[i].common.c3
            + (rs->vertices[i + 1].common.c3 - rs->vertices[i].common.c3)
            * fraction;

        //TERRAIN_SetToGroundLevel(m_terrainData, position);
    }
    else
    {
        for (i = 0; i <= rs->num_vertices - 2; i++)
        {
            COORD_CalcDistance(m_terrainData->getCoordinateSystem(),
                               &(rs->vertices[rs->num_vertices - i - 1]),
                               &(rs->vertices[rs->num_vertices - i - 2]),
                               &distance);
            if (accumulated_distance + distance <= traveling_distance)
            {
                accumulated_distance += distance;
            }
            else
            {
                break;
            }
        }

        if (i == rs->num_vertices - 1)
        {
            ERROR_ReportError("ENVIRONMENT: tried to travel farther down "
                              "road than length of road\n");
        }

        double fraction
        = (traveling_distance - accumulated_distance) / distance;

        position->common.c1 =
            rs->vertices[rs->num_vertices - i - 1].common.c1
            + (rs->vertices[rs->num_vertices - i - 2].common.c1
               - rs->vertices[rs->num_vertices - i - 1].common.c1)
            * fraction;

        position->common.c2 =
            rs->vertices[rs->num_vertices - i - 1].common.c2
            + (rs->vertices[rs->num_vertices - i - 2].common.c2
               - rs->vertices[rs->num_vertices - i - 1].common.c2)
            * fraction;

        position->common.c3 =
            rs->vertices[rs->num_vertices - i - 1].common.c3
            + (rs->vertices[rs->num_vertices - i - 2].common.c3
               - rs->vertices[rs->num_vertices - i - 1].common.c3)
            * fraction;

    }
}

std::string* QualNetUrbanTerrainData::fileList(NodeInput *nodeInput)
{
    //Array of strings containing the files names of all the XML
    //files to be parsed
    BOOL wasFound;

    std::string* response = new std::string("");

    int numFiles = 0;

    char buf[MAX_STRING_LENGTH];

    IO_ReadString(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "TERRAIN-FEATURES-SOURCE",
        &wasFound,
        buf);

    if (!wasFound) {
        return NULL;
    }
    else {
        *response += " ";
        *response += buf;

        if (strcmp(buf, "FILE") == 0) {
            IO_ReadString(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "TERRAIN-FEATURES-FILELIST",
                &wasFound,
                buf);

            if (wasFound)
            {
                delete response;
                return new std::string(""); // this isn't supported currently
            }
            else
            {
                while (TRUE) {
                    BOOL fallBackToGlobal = FALSE;
                    if (numFiles == 0) { fallBackToGlobal = TRUE; }

                    IO_ReadStringInstance(
                        ANY_NODEID,
                        ANY_ADDRESS,
                        nodeInput,
                        "TERRAIN-FEATURES-FILENAME",
                        numFiles,
                        fallBackToGlobal,
                        &wasFound,
                        buf);

                    if (!wasFound) {
                        break;
                    }
                    numFiles++;
                    *response += " ";
                    *response += buf;
                }
            }
        }
#ifdef CTDB7_INTERFACE
        else if (strcmp(buf, "CTDB7") == 0) {
            // this isn't supported currently
        }
#endif // CTDB7_INTERFACE
#ifdef CTDB8_INTERFACE
        else if (strcmp(buf, "CTDB8") == 0) {
            // this isn't supported currently
        }
#endif // CTDB8_INTERFACE
//#ifdef ESRI_SHP_INTERFACE
        else if (strcmp(buf, "SHAPEFILE") == 0) {

            char shapefilePath[MAX_STRING_LENGTH];
            char shapefileType[MAX_STRING_LENGTH];
            char defaultMsmtUnit[MAX_STRING_LENGTH];
            char defaultHeight[MAX_STRING_LENGTH];
            char defaultFoliageDensity[MAX_STRING_LENGTH];
            char dbfMsmtUnit[MAX_STRING_LENGTH];
            char dbfHeightTag[MAX_STRING_LENGTH];
            char dbfDensityTag[MAX_STRING_LENGTH];

            char thisFile[MAX_STRING_LENGTH];

            while (TRUE) {
                BOOL fallBackToGlobal = FALSE;
                if (numFiles == 0) { fallBackToGlobal = TRUE; }

                // SHAPEFILE-PATH is required
                IO_ReadStringInstance(
                    ANY_NODEID,
                    ANY_ADDRESS,
                    nodeInput,
                    "SHAPEFILE-PATH",
                    numFiles,
                    fallBackToGlobal,
                    &wasFound,
                    shapefilePath);

                if (!wasFound)
                {
                    break;
                }

                IO_ReadStringInstance(
                    ANY_NODEID,
                    ANY_ADDRESS,
                    nodeInput,
                    "SHAPEFILE-DEFAULT-SHAPE-TYPE",
                    numFiles,
                    TRUE,
                    &wasFound,
                    shapefileType);

                if (!wasFound)
                {
                    strcpy(shapefileType, "BUILDING");
                }

                IO_ReadStringInstance(
                    ANY_NODEID,
                    ANY_ADDRESS,
                    nodeInput,
                    "SHAPEFILE-DEFAULT-MSMT-UNIT",
                    numFiles,
                    TRUE,
                    &wasFound,
                    defaultMsmtUnit);

                if (!wasFound)
                {
                    strcpy(defaultMsmtUnit, "FEET");
                }

                IO_ReadStringInstance(
                    ANY_NODEID,
                    ANY_ADDRESS,
                    nodeInput,
                    "SHAPEFILE-DBF-FILE-MSMT-UNIT",
                    numFiles,
                    TRUE,
                    &wasFound,
                    dbfMsmtUnit);

                if (!wasFound)
                {
                    strcpy(dbfMsmtUnit, "FEET");
                }

                if (!strcmp(shapefileType, "BUILDING")) {
                    IO_ReadStringInstance(
                        ANY_NODEID,
                        ANY_ADDRESS,
                        nodeInput,
                        "SHAPEFILE-DEFAULT-BLDG-HEIGHT",
                        numFiles,
                        TRUE,
                        &wasFound,
                        defaultHeight);

                    if (!wasFound)
                    {
                        strcpy(defaultHeight, "35");
                    }

                    IO_ReadStringInstance(
                        ANY_NODEID,
                        ANY_ADDRESS,
                        nodeInput,
                        "SHAPEFILE-DBF-BLDG-HEIGHT-TAG-NAME",
                        numFiles,
                        TRUE,
                        &wasFound,
                        dbfHeightTag);

                    if (!wasFound)
                    {
                        strcpy(dbfHeightTag, "LV");
                    }

                    sprintf(thisFile, " \"%s\" %s %s %s %s %s",
                            shapefilePath,
                            shapefileType,
                            defaultMsmtUnit,
                            defaultHeight,
                            dbfMsmtUnit,
                            dbfHeightTag);
                    *response += thisFile;
                }
                else { // FOLIAGE
                    IO_ReadStringInstance(
                        ANY_NODEID,
                        ANY_ADDRESS,
                        nodeInput,
                        "SHAPEFILE-DEFAULT-FOLIAGE-HEIGHT",
                        numFiles,
                        TRUE,
                        &wasFound,
                        defaultHeight);

                    if (!wasFound)
                    {
                        strcpy(defaultHeight, "35");
                    }

                    IO_ReadStringInstance(
                        ANY_NODEID,
                        ANY_ADDRESS,
                        nodeInput,
                        "SHAPEFILE-DEFAULT-FOLIAGE-DENSITY",
                        numFiles,
                        TRUE,
                        &wasFound,
                        defaultFoliageDensity);

                    if (!wasFound)
                    {
                        strcpy(defaultFoliageDensity, "0.15");
                    }

                    IO_ReadStringInstance(
                        ANY_NODEID,
                        ANY_ADDRESS,
                        nodeInput,
                        "SHAPEFILE-DBF-FOLIAGE-HEIGHT-TAG-NAME",
                        numFiles,
                        TRUE,
                        &wasFound,
                        dbfHeightTag);

                    if (!wasFound)
                    {
                        strcpy(dbfHeightTag, "LV");
                    }

                    IO_ReadStringInstance(
                        ANY_NODEID,
                        ANY_ADDRESS,
                        nodeInput,
                        "SHAPEFILE-DBF-FOLIAGE-DENSITY-TAG-NAME",
                        numFiles,
                        TRUE,
                        &wasFound,
                        dbfDensityTag);


                    if (!wasFound)
                    {
                        strcpy(dbfDensityTag, "DENSITY");
                    }

                    sprintf(thisFile, " \"%s\" %s %s %s %s %s %s %s",
                            shapefilePath,
                            shapefileType,
                            defaultMsmtUnit,
                            defaultHeight,
                            dbfMsmtUnit,
                            dbfHeightTag,
                            defaultFoliageDensity,
                            dbfDensityTag);

                    *response += thisFile;
                }
                numFiles++;
            }

        }
//#endif // ESRI_SHP_INTERFACE
        else {
            ERROR_ReportError("Unknown TERRAIN-FEATURES-SOURCE\n");
        }
    }

    return response;
}

void QualNetUrbanTerrainData::groupSignals()
{
    int i, j, n;
    double cosine;

    for (n = 0; n < m_numIntersections; n++)
    {
        if (intersectionHasTrafficSignal(n) == true)
        {
            const Coordinates* intersectionLoc =
                intersectionLocation(n);

            const IntersectionID* neighbors;
            int numNeighbors;
            neighboringIntersections(n, &neighbors, &numNeighbors);

            for (i = 0; i < numNeighbors; i++)
            {
                m_intersections[n].signalGroups[i] = -1;
            }

            for (i = 0; i < numNeighbors; i++)
            {
                if (m_intersections[n].signalGroups[i] == -1)
                {
                    m_intersections[n].signalGroups[i] = i;
                }

                const Coordinates* firstNeighborLocation =
                    intersectionLocation(neighbors[i]);

                for (j = i + 1; j < numNeighbors; j++)
                {
                    if (m_intersections[n].signalGroups[j] == -1)
                    {
                        const Coordinates* secondNeighborLocation =
                            intersectionLocation(neighbors[j]);
                        cosine = cosineOfVectors(
                                     m_terrainData->getCoordinateSystem(),
                                     intersectionLoc,
                                     firstNeighborLocation,
                                     secondNeighborLocation);

                        if (cosine < -sqrt(3.0) * 0.5)
                        {
                            m_intersections[n].signalGroups[j] = m_intersections[n].signalGroups[i];
                        }
                    }
                }
            }

        }
    }
}


double QualNetUrbanTerrainData::calculateBuildingHeight(const Building* building)
{
    double tmp;

    FeatureFace* face;
    int i, j;

    double buildingHeight =
        fabs(building->faces[0].vertices[0].common.c3
             - building->faces[0].vertices[building->faces[0].num_vertices-1].common.c3);

    for (i = 0; i < building->num_faces; i++)
    {
        face = &building->faces[i];

        tmp = fabs(face->vertices[0].common.c3
                   - face->vertices[face->num_vertices-1].common.c3);

        if (tmp > buildingHeight)
        {
            buildingHeight = tmp;
        }

        for (j = 1; j < face->num_vertices; j++)
        {
            tmp = fabs(face->vertices[j].common.c3
                       - face->vertices[j-1].common.c3);
            if (tmp > buildingHeight)
            {
                buildingHeight = tmp;
            }
        }
    }
    return buildingHeight;
}


void QualNetUrbanTerrainData::addHeightInformation(Building* building,
        float* minBuildingHeight,
        float* maxBuildingHeight,
        float* totalBuildingHeight)
{
    building->height = calculateBuildingHeight(building);

    if (building->height < *minBuildingHeight)
    {
        *minBuildingHeight = (float)building->height;
    }
    if (building->height > *maxBuildingHeight)
    {
        *maxBuildingHeight = (float)building->height;
    }
    *totalBuildingHeight += (float)building->height;
}


void QualNetUrbanTerrainData::convertBuildingCoordinates(
    Building* building,
    CoordinateRepresentationType type)
{
    FeatureFace* face;
    int i;
    int j;

    Coordinates tmp;

    for (i = 0; i < building->num_faces; i++)
    {
        face = &building->faces[i];

        for (j = 0; j < face->num_vertices; j++)
        {
            memcpy(&tmp, &(face->vertices[j]), sizeof(Coordinates));
            tmp.type = GEODETIC;
            COORD_ChangeCoordinateSystem(&tmp,
                                         type,
                                         &(face->vertices[j]));
        }
    }
}

void QualNetUrbanTerrainData::setTerrainFeaturesToGround(Building* features,
        int numFeatures)
{
    int i, j, k;
    for (i = 0; i < numFeatures; i++)
    {
        // Get ground elevation for first vertex of first face of obstruction.
        // TODO:  For best results (no floating buildings/foliage), should
        // obtain minimum ground elevation across all vertices for a given
        // obstruction.
        Building* feature = &features[i];

        Coordinates featurePositionGround;
        memcpy(&featurePositionGround,
               &feature->faces[0].vertices[0],
               sizeof(featurePositionGround));
        TERRAIN_SetToGroundLevel(m_terrainData, &featurePositionGround);

        double groundElevation = featurePositionGround.common.c3;

        if (NODEBUG) {
            printf("Feature %u ground %.1f\n",
                   i, featurePositionGround.common.c3);
        }

        // Update all feature vertices with new height.
        for (j = 0; j < feature->num_faces; j++)
        {
            FeatureFace* face = &feature->faces[j];

            for (k = 0; k < face->num_vertices; k++)
            {
                face->vertices[k].common.c3 += groundElevation;
            }
        }//for//
    }//for//
}


void QualNetUrbanTerrainData::returnIntersectionBuildings(
    Building* features,
    int numFeatures,
    const Coordinates* source,
    const Coordinates* dest,
    int* const numResults,
    BuildingID** const buildings,
    Coordinates (** const intersections)[2],
    FaceIndex (** const faces)[2])
{
    int i;
    int maxResults = 100;

    bool isOverlap;
    Coordinates intersection1;
    Coordinates intersection2;

    FaceIndex face1;
    FaceIndex face2;

    *numResults = 0;

    if (buildings)
    {
        (*buildings) = (BuildingID*) MEM_malloc(maxResults * sizeof(BuildingID));
    }
    if (intersections)
    {
        (*intersections) = (Coordinates(*)[2])
            MEM_malloc(maxResults * sizeof(Coordinates) * 2);
    }
    if (faces)
    {
        (*faces) = (FaceIndex(*)[2])
            MEM_malloc(maxResults * sizeof(FaceIndex) * 2);
    }

    if (NODEBUG) {
        printf("checking line segment (%f,%f,%f), (%f,%f,%f)\n",
               source->cartesian.x,
               source->cartesian.y,
               source->cartesian.z,
               dest->cartesian.x,
               dest->cartesian.y,
               dest->cartesian.z);
    }

    for (i = 0; i < numFeatures; i++)
    {
        isOverlap = calculateOverlap(
            m_terrainData->getCoordinateSystem(),
            source,
            dest,
            &(features[i]),
            &intersection1,
            &intersection2,
            &face1,
            &face2);

        if (isOverlap)
        {
            // TBD, results are in GEOCENTRIC_CARTESIAN, so 
            // why not leave them in GCC?
            if (m_terrainData->getCoordinateSystem() == LATLONALT)
            {
                Coordinates save1;
                Coordinates save2;

                COORD_ChangeCoordinateSystem(
                    &intersection1, GEODETIC, &save1);
                COORD_ChangeCoordinateSystem(
                    &intersection2, GEODETIC, &save2);

                memcpy(&intersection1, &save1, sizeof(Coordinates));
                memcpy(&intersection2, &save2, sizeof(Coordinates));
            }

            // out of space 
            if (*numResults == maxResults)
            {
                if (buildings)
                {
                    BuildingID* tmpBuildings = (*buildings);

                    (*buildings) = (BuildingID*)
                        MEM_malloc(2 * maxResults * sizeof(BuildingID));

                    memcpy((*buildings),
                           tmpBuildings, maxResults * sizeof(BuildingID));

                    MEM_free(tmpBuildings);
                }

                if (intersections)
                {
                    Coordinates (* tmpIntersections)[2] = (*intersections);

                    (*intersections) = (Coordinates(*)[2])
                        MEM_malloc(2 * maxResults * sizeof(Coordinates) * 2);

                    memcpy((*intersections),
                           tmpIntersections, maxResults * sizeof(Coordinates) * 2);

                    MEM_free(tmpIntersections);
                }

                if (faces)
                {
                    FaceIndex (* tmpFaces)[2] = (*faces);

                    (*faces) = (FaceIndex(*)[2])
                        MEM_malloc(2 * maxResults * sizeof(FaceIndex) * 2);

                    memcpy((*faces), tmpFaces, maxResults * sizeof(FaceIndex) * 2);

                    MEM_free(tmpFaces);
                }
                maxResults *= 2;
            }

            if (buildings)
            {
                (*buildings)[*numResults] = i;
            }

            if (intersections)
            {
                memcpy(&((*intersections)[*numResults][0]),
                       &intersection1,
                       sizeof(Coordinates));
                memcpy(&((*intersections)[*numResults][1]),
                       &intersection2,
                       sizeof(Coordinates));
            }

            if (faces)
            {
                (*faces)[*numResults][0] = face1;
                (*faces)[*numResults][1] = face2;
            }

            (*numResults)++;
        }
    }
}

















