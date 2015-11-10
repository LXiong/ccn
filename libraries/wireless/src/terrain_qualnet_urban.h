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

#ifndef QUALNET_URBAN_TERRAIN_H
#define QUALNET_URBAN_TERRAIN_H

#include <list>
#include <map>
#include <set>
#include <vector>

#include "geometry.h"
#include "simplesplay.h"
#include "terrain.h"

#define SMALL_NUM 0.0001

/*! \def INVALID_INTERSECTION
  \brief A constant used to tag a coordinate as one
  that does not lie at a street intersection
*/
#define INVALID_INTERSECTION -1

/*! \def INVALID_PARK_ID
  \brief A constant used to tag the ParkID at an
  intersection as one that does not exist
*/
#define INVALID_PARK_ID -1

/*! \def INVALID_STATION_ID
  \brief A constant used to tag the StationID at an
  intersection as one that does not exist
*/
#define INVALID_STATION_ID -1

#define COSINE_ERROR_FIRST_SECOND -3
#define COSINE_ERROR_FIRST_THIRD -2
#define COSINE_ERROR_ALL -1

struct PlaneParameters
{
    double normalX;
    double normalY;
    double normalZ;
    double d;
};

struct FeatureFace
{
    char* XML_ID;
    int num_vertices;
    Coordinates* vertices;
    double thickness;
    Cube boundingCube;
    PlaneParameters plane;

    void print() const {
        printf("Face %s\n", XML_ID);
        for (int i = 0; i < num_vertices; i++) {
            printf("\t(%f,%f,%f)\n",
                   vertices[i].common.c1,
                   vertices[i].common.c2,
                   vertices[i].common.c3);
        }
    }
};

struct FeatureGridInfo
{
    int numBoxes_BuildingBelongsTo;
    int* listBoxes_BuildingBelongsTo;
    int gridBox_WithBuildingData;
    int BldgIdxIn_gridBox_WithBuildingData;
};

struct Building
{
    char* XML_ID;
    char* featureName;
    int num_faces;
    FeatureFace* faces;
    Cube   boundingCube; // in GCC
    Box    footprint;    // in the scenario coordinate system
    double height;
    double density; // for foliage
    FeatureGridInfo* featureGridInfo;

    double getCoverageArea(int coordinateSystem) const {
        return footprint.getArea(coordinateSystem);
    }
        
    void print() const { printf("building %s\n", XML_ID); }
};

typedef Building Foliage;

typedef int BuildingID;
typedef int FaceIndex;
typedef int FoliageID;
typedef int IntersectionID;
typedef int ParkID;
typedef int ParkStationID;
typedef int RoadID;
typedef int RoadSegmentID;
typedef int SignalGroupNumber;
typedef int StationID;

#define FACE_RECEIVER -1
#define FACE_TRANSMITTER -2

struct RoadSegment
{
    char* XML_ID;
    char* streetName;
    double width;
    double length;
    int num_vertices;
    Coordinates* vertices;
    RoadSegmentID ID;

    // intersection associated with vertices[0]
    IntersectionID firstIntersection;
    // intersection associated with vertices[num_vertices-1]
    IntersectionID secondIntersection;

    // Users should not modify anything above this line after initialization

    // All dynamic data should be managed by user in user-defined structure
    void* roadSegmentVar; // should set default NULL
    ~RoadSegment() {
        if (XML_ID         != NULL) delete XML_ID;
        if (streetName     != NULL) delete streetName;
        if (vertices       != NULL) delete vertices;
        //if (roadSegmentVar != NULL) delete roadSegmentVar;
    }
};

enum EntranceType
{
    ENTRANCE_NONE = 0,
    ENTRANCE_STATION,
    ENTRANCE_PARK
};


struct Intersection
{
    char* XML_ID;
    IntersectionID ID;
    Coordinates position;
    RoadSegmentID* roadSegments;
    int num_road_segments;
    // or could do fixed maximum
    IntersectionID* neighbors;
    // each corresponds to a road segment

    EntranceType entranceType;
    ParkID park;
    StationID station;

    // traffic signals
    bool hasTrafficSignal;
    int numSignalGroups;
    SignalGroupNumber* signalGroups;

    void* intersectionVar;

    ~Intersection() {
        if (XML_ID          != NULL) delete XML_ID;
        if (roadSegments    != NULL) delete roadSegments;
        if (neighbors       != NULL) delete neighbors;
        if (signalGroups    != NULL) delete signalGroups;
        //if (intersectionVar != NULL) delete intersectionVar;
    }
};

// used to fill dummy in Intersection during parsing of XML
struct TrafficLight
{
    SignalGroupNumber signalGroup;
    RoadSegmentID roadID;
};

struct Station
{
    char* XML_ID;
    char* stationName;
    StationID ID;
    // Coordinates position;

    int num_vertices;
    Coordinates* vertices;
    IntersectionID* exitIntersections;
    Coordinates representative;
    void* stationVar;

    ~Station() {
        if (XML_ID            != NULL) delete XML_ID;
        if (stationName       != NULL) delete stationName;
        if (vertices          != NULL) delete vertices;
        if (exitIntersections != NULL) delete exitIntersections;
        //if (stationVar        != NULL) delete stationVar;
    }
};

struct ParkStation
{
    char* XML_ID;
    char* parkStationName;
    ParkStationID ID;
    // Coordinates position;

    int num_vertices;
    Coordinates* vertices;
    // one for each vertex
    // value is IntersectionID or INVALID_INTERSECTION
    IntersectionID* exitIntersections;
    Coordinates representative;
    void* parkStationVar;
    void* parkStationGridVar;
    // indicate size of grid
    // indicate number of grid elements

    ~ParkStation() {
        if (XML_ID             != NULL) delete XML_ID;
        if (parkStationName    != NULL) delete parkStationName;
        if (vertices           != NULL) delete vertices;
        if (exitIntersections  != NULL) delete exitIntersections;
        //if (parkStationVar     != NULL) delete parkStationVar;
        //if (parkStationGridVar != NULL) delete parkStationGridVar;
    }
};

typedef ParkStation Park;

typedef LineSegment IntersectedPoints;

struct IntersectedFaces {
    FaceIndex f1;
    FaceIndex f2;
};

struct BuildingData {
    // always put min distance first, as measured from source
    BuildingID building;
    double distance1;
    double distance2;
    Coordinates c1; //! first intersection
    Coordinates c2; //! 2nd intersection
    FaceIndex f1; //! first face
    FaceIndex f2; //! 2nd face

    bool operator<  (const BuildingData& other) const {
        return (distance1 <= other.distance1);
    }
};

struct FoliageData {
    BuildingID    foliage;
    double        distanceThrough;
    double        density;
    FoliatedState foliatedState;
};

struct TerrainPathData {
    int numBuildings;
    std::set<BuildingID> buildingIDs;
    std::map<BuildingID, IntersectedPoints> buildingIntersections;
    std::map<BuildingID, IntersectedFaces>  buildingFaces;

    int numFoliage;
    std::set<BuildingID> foliageIDs;
    std::map<BuildingID, IntersectedPoints> foliageIntersections;
    std::map<BuildingID, IntersectedFaces>  foliageFaces;

    TerrainPathData() {
        numBuildings = 0;
        numFoliage = 0;
    }
};

class QualNetUrbanTerrainData;

class QualNetUrbanPathProperties : public UrbanPathProperties {
private:
    QualNetUrbanTerrainData* m_urbanData;
    TerrainPathData*                m_pathData;

    //! sorted, collated version of the lists in PathData
    std::vector<BuildingData> m_buildingList;
    std::vector<FoliageData>  m_foliageList;

    Coordinates m_sourceGCC;
    Coordinates m_destGCC;

    bool m_alreadySorted;
    bool m_alreadySortedFoliage;
    bool m_alreadyCalculatedBuildingDistances;
    bool m_alreadyCalculatedBuildingHeights;
    bool m_alreadyCalculatedSrcDistanceToIntersection;
    bool m_alreadyCalculatedDestDistanceToIntersection;
    bool m_alreadyCalculatedIntersectionAngle;
    bool m_alreadyCalculatedRelativeOrientation;

    void sortBuildings();
    void sortFoliage();
    void calculateBuildingHeights();
    void calculateBuildingDistances();
public:
    QualNetUrbanPathProperties(QualNetUrbanTerrainData* urbanData,
                               const Coordinates* c1,
                               const Coordinates* c2,
                               const int coordinateSystem,
                               const Coordinates* c1gcc,
                               const Coordinates* c2gcc) :
        UrbanPathProperties(c1, c2, coordinateSystem)
    {
        m_pathData  = NULL;
        m_urbanData = urbanData;
        m_sourceGCC = *c1gcc;
        m_destGCC   = *c2gcc;

        m_alreadySorted        = false;
        m_alreadySortedFoliage = false;

        m_alreadyCalculatedBuildingDistances          = false;
        m_alreadyCalculatedBuildingHeights            = false;
        m_alreadyCalculatedSrcDistanceToIntersection  = false;
        m_alreadyCalculatedDestDistanceToIntersection = false;
        m_alreadyCalculatedIntersectionAngle          = false;
        m_alreadyCalculatedRelativeOrientation        = false;

        m_buildingList.clear();
        m_foliageList.clear();
    }
    ~QualNetUrbanPathProperties() {
        m_buildingList.clear();
        m_foliageList.clear();
        if (m_pathData != NULL) delete m_pathData;
    }

    double getAvgBuildingSeparation() {
        calculateBuildingDistances();
        return m_avgBuildingSeparation;
    }
    double getAvgBuildingHeight() {
        calculateBuildingHeights();
        return m_avgBuildingHeight;
    }
    double getAvgStreetWidth() {
        calculateBuildingDistances();
        return m_avgStreetWidth;
    }
    double getSrcDistanceToBuilding() {
        calculateBuildingDistances();
        return m_srcDistanceToBuilding;
    }
    double getDestDistanceToBuilding() {
        calculateBuildingDistances();
        return m_destDistanceToBuilding;
    }
    double getSrcDistanceToIntersection();
    double getDestDistanceToIntersection();
    double getIntersectionAngle();
    double getRelativeOrientation();
    double getMaxRoofHeight() {
        calculateBuildingHeights();
        return m_maxRoofHeight;
    }
    double getMinRoofHeight() {
        calculateBuildingHeights();
        return m_minRoofHeight;
    }
    double getAvgFoliageHeight();

    double getDistanceThroughBuilding(const int b) {
        sortBuildings();
        BuildingData data = m_buildingList[b];
        return (data.distance2 - data.distance1);
    }
    double getDistanceThroughFoliage(const int f) {
        sortFoliage();
        return m_foliageList[f].distanceThrough;
    }
    double getFoliageDensity(const int f) {
        sortFoliage();
        return m_foliageList[f].density;
    }
    FoliatedState getFoliatedState(const int f) {
        sortFoliage();
        return m_foliageList[f].foliatedState;
    }

    void setPathData(TerrainPathData* p) { m_pathData = p; }
    void print();
};

class QualNetUrbanTerrainData : public UrbanTerrainData
{
public:
    Coordinates m_southWest;           /*!< Coordinates of south-western
                                                most point*/
    Building * m_buildings;            /*!< Array of buildings parsed*/
    Foliage * m_foliage;               /*!< Array of foliage parsed*/
    RoadSegment * m_roadSegments;      /*!< Array of road segments parsed*/
    Intersection * m_intersections;    /*!< Array of intersections parsed*/
    Park * m_parks;                    /*!< Array of Parks parsed*/
    Station * m_stations;              /*!< Array of Stations parsed*/
    int  m_numIntersections;           /*!< Number of intersections parsed*/
    int  m_numRoadSegments;            /*!< Number of road segments parsed*/
    int  m_numBuildings;               /*!< Number of buildings parsed*/
    int  m_numFoliage;                 /*!< Number of foliage parsed*/
    int  m_numParks;                   /*!< Number of parks parsed*/
    int  m_numStations;                /*!< Number of stations parsed*/
    int  m_numClouds;                  /*!< Number of clouds parsed*/
    int  m_numRailroads;               /*!< Number of railroads parsed*/

    float  m_totalBuildingHeight;
    float  m_minBuildingHeight;
    float  m_maxBuildingHeight;

    float  m_totalFoliageHeight;
    float  m_minFoliageHeight;
    float  m_maxFoliageHeight;

    FoliatedState  m_foliatedState;

    void*  m_roadSegmentVar;           /*!< Array for dynamic data for roads*/
    size_t m_roadElementSize;
    void*  m_parkStationVar;           /*!< Array of dynamic data for parks*/
    size_t m_parkStationElementSize;
    SimpleSplayTree m_pedestrianSplayTree;

    bool  m_setFeaturesToGround;

    QualNetUrbanTerrainData(TerrainData* td) : UrbanTerrainData(td) {
        m_modelName = "QUALNET-URBAN";

        m_buildings     = NULL;
        m_foliage       = NULL;
        m_roadSegments  = NULL;
        m_intersections = NULL;
        m_parks         = NULL;
        m_stations      = NULL;

        m_numIntersections = 0;
        m_numRoadSegments  = 0;
        m_numBuildings     = 0;
        m_numFoliage       = 0;
        m_numParks         = 0;
        m_numStations      = 0;
        m_numClouds        = 0;
        m_numRailroads     = 0;

        m_totalBuildingHeight = 0.0;
        m_minBuildingHeight   = 0.0;
        m_maxBuildingHeight   = 0.0;
        m_totalFoliageHeight  = 0.0;
        m_minFoliageHeight    = 0.0;
        m_maxFoliageHeight    = 0.0;

        m_foliatedState = IN_LEAF;

        m_roadSegmentVar         = NULL;
        m_roadElementSize        = 0;
        m_parkStationVar         = NULL;
        m_parkStationElementSize = 0;
        m_setFeaturesToGround    = false;
    }
    ~QualNetUrbanTerrainData();

    void initialize(NodeInput* nodeInput,
                    bool       masterProcess = false);
    void finalize();

    bool hasData() { return true; }
    void populateRegionData(TerrainRegion* region,
                            TerrainRegion::Mode mode);

    UrbanPathProperties* getUrbanPathProperties(const  Coordinates* c1,
            const  Coordinates* c2,
            double pathWidth,
            bool   includeFoliage = false);

    int  getNumberOfParks()         { return m_numParks; }
    int  getNumberOfStations()      { return m_numStations; }
    int  getNumberOfIntersections() { return m_numIntersections; }
    int  getNumberOfRoadSegments()  { return m_numRoadSegments; }

    void createBoundingCube(Building* building);

    void createPlaneParameters(Building* building);

    void calculateIntersection3Planes(
        const FeatureFace* face1,
        const FeatureFace* face2,
        const FeatureFace* face3,
        Coordinates* const intersection);

    double cosineOfVectors(
        int coordinateSystemType,
        const Coordinates* source,
        const Coordinates* dest1,
        const Coordinates* dest2);

    double calcDistanceOverTerrain(
        const Coordinates* position1,
        const Coordinates* position2,
        double elevationSamplingDistance);

    void roadGiveIntermediatePosition(
        Coordinates* const position,
        RoadSegmentID ID,
        IntersectionID startIntersection,
        double traveling_distance);

//
// RoadSegment functions
//

    RoadSegment* getRoadSegment(RoadSegmentID ID) {
        return &(m_roadSegments[ID]);
    }

    double roadSegmentArea(RoadSegmentID ID) {
        RoadSegment* rs = getRoadSegment(ID);
        return rs->width * rs->length;
    }

    double roadSegmentWidth(RoadSegmentID ID) {
        RoadSegment* rs = getRoadSegment(ID);
        return rs->width;
    }

    double roadSegmentLength(RoadSegmentID ID) {
        RoadSegment* rs = getRoadSegment(ID);
        return rs->length;
    }

    void allocateAllRoadSegmentVars(size_t numBytes) {
        m_roadSegmentVar  = MEM_malloc(m_numRoadSegments * numBytes);
        m_roadElementSize = numBytes;
    }

    void* returnRoadSegmentVar(RoadSegmentID ID) {
        if (m_roadSegmentVar == NULL)
        {
            ERROR_ReportWarning(
                "ReturnRoadSegmentVar: User data has not yet "
                "been allocated.\n");
        }

        // assumes that char are one byte
        return (char*) m_roadSegmentVar + (m_roadElementSize * ID);
    }

    void freeRoadSegmentVar() {
        if (m_roadSegmentVar != NULL)
        {
            MEM_free(m_roadSegmentVar);
            m_roadSegmentVar = NULL;
        }
        else
        {
            ERROR_ReportWarning(
                "FreeRoadSegmentVar: User data has not been allocated.\n");
        }
    }

    void intersectionsOfRoadSegment(RoadSegmentID ID,
                                    IntersectionID* firstIntersection,
                                    IntersectionID* secondIntersection) {

        RoadSegment* rs = getRoadSegment(ID);
        *firstIntersection = rs->firstIntersection;
        *secondIntersection = rs->secondIntersection;
    }

    // sets points to point to location of vertices where they are
    // actually stored for the terraindata
    void RoadSegmentPoints(RoadSegmentID ID,
                           const Coordinates** points,
                           int* const num_points) {
        RoadSegment* rs = getRoadSegment(ID);
        *points = (const Coordinates*) rs->vertices;
        *num_points = rs->num_vertices;
    }

    //
    // Intersection functions
    //

    Intersection* getIntersection(IntersectionID ID) {
        return &(m_intersections[ID]);
    }

    // sets roadSegments to point to location of adjacent road segments
    // where they are actually stored for the terraindata
    void adjacentRoadSegments(IntersectionID ID,
                              const RoadSegmentID** roadSegments,
                              int* num_road_segments) {
        Intersection* i = getIntersection(ID);
        *roadSegments = (const RoadSegmentID*) i->roadSegments;
        *num_road_segments = i->num_road_segments;
    }

    // sets neighbors to point to location of adjacent intersection
    // where they are actually stored for the terraindata
    void neighboringIntersections(IntersectionID ID,
                                  const IntersectionID** neighbors,
                                  int* num_neighbors) {
        Intersection* i = getIntersection(ID);
        *neighbors = (const IntersectionID*) i->neighbors;
        *num_neighbors = i->num_road_segments;
    }

    void trafficSignalGroups(IntersectionID ID,
                             const SignalGroupNumber** signalGroups,
                             int* num_neighbors) {
        Intersection* i = getIntersection(ID);
        *signalGroups = (const IntersectionID*) i->signalGroups;
        *num_neighbors = i->num_road_segments;
    }

    EntranceType intersectionEntranceType(IntersectionID ID) {
        Intersection* i = getIntersection(ID);
        return i->entranceType;
    }

    int numberOfNeighboringIntersections(IntersectionID ID) {
        Intersection* i = getIntersection(ID);
        return i->num_road_segments;
    }

    // returns const* to coordinates of intersection
    const Coordinates* intersectionLocation(IntersectionID ID) {
        Intersection* i = getIntersection(ID);
        return (const Coordinates*) &(i->position);
    }

    ParkID intersectionPark(IntersectionID ID) {
        Intersection* i = getIntersection(ID);
        return i->park;
    }

    bool intersectionHasTrafficSignal(IntersectionID ID) {
        Intersection* i = getIntersection(ID);
        return i->hasTrafficSignal;
    }

    // ParkStationFunctions

    ParkStation* getParkStation(ParkStationID ID) { return &(m_parks[ID]); }
    void allocateAllParkStationVars(size_t numBytes) {
        m_parkStationVar         = MEM_malloc(m_numParks * numBytes);
        m_parkStationElementSize = numBytes;
    }

    void* returnParkStationVar(ParkStationID ID) {
        if (m_parkStationVar == NULL)
        {
            ERROR_ReportWarning(
                "ReturnParkStationVar: User data has not yet "
                "been allocated.\n");
        }

        // assumes that char are one byte
        return (char*) m_parkStationVar + (m_parkStationElementSize * ID);
    }

    void freeParkStationVar() {
        if (m_parkStationVar != NULL)
        {
            MEM_free(m_parkStationVar);
            m_parkStationVar = NULL;
        }
        else
        {
            ERROR_ReportWarning(
                "FreeParkStationVar: User data has not been allocated.\n");
        }
    }

    // sets points to point to location of vertices where they are
    // actually stored for the terraindata
    void parkStationPoints(ParkStationID ID,
                           const Coordinates** points,
                           int* const num_points) {
        ParkStation* ps = getParkStation(ID);
        *points = (const Coordinates*) ps->vertices;
        *num_points = ps->num_vertices;
    }


    void parkStationEntrances(ParkStationID ID,
                              const IntersectionID** exits,
                              int* const num_points) {
        ParkStation* ps = getParkStation(ID);
        *exits = (const IntersectionID*) ps->exitIntersections;
        *num_points = ps->num_vertices;
    }

    const Coordinates* parkStationRepresentativeCoordinate(ParkStationID ID) {
        ParkStation* ps = getParkStation(ID);
        return (const Coordinates*) &(ps->representative);
    }

    double getAvgHRoof() { return (m_totalBuildingHeight / m_numBuildings); }
    double getMaxBuildingHeight() { return m_maxBuildingHeight; }
    double getMinBuildingHeight() { return m_minBuildingHeight; }

    std::string* fileList(NodeInput *nodeInput);

    void groupSignals();

    double calculateBuildingHeight(const Building* building);

    void addHeightInformation(Building* building,
                              float* minBuildingHeight,
                              float* maxBuildingHeight,
                              float* totalBuildingHeight);

    void convertBuildingCoordinates(Building* building,
                                    CoordinateRepresentationType type);

    void setTerrainFeaturesToGround(Building* features,
                                    int numFeatures);

private:
    std::list<LineSegment>* constructPath(const  Coordinates* c1,
                                          const  Coordinates* c2,
                                          double pathWidth);
    TerrainPathData* getFeaturesOnPath(std::list<LineSegment>* segmentList,
                                bool includeFoliage);

    void returnIntersectionBuildings(
                   Building* features,
                   int numFeatures,
                   const Coordinates* source,
                   const Coordinates* dest,
                   int* const numResults,
                   BuildingID** const buildings,
                   Coordinates (** const intersections)[2],
                   FaceIndex (** const faces)[2]);

    bool calculateOverlap(
        int coordinateSystemType,
        const Coordinates* source,
        const Coordinates* dest,
        Building* obstruction,
        Coordinates* const intersection1,
        Coordinates* const intersection2,
        FaceIndex* const face1,
        FaceIndex* const face2);

    bool calculateIntersectionSegmentPlane(
        int coordinateSystemType,
        const Coordinates* source,
        const Coordinates* dest,
        const FeatureFace* face,
        Coordinates* const intersection);

    bool passThroughTest(
        const Coordinates* source,
        const Coordinates* dest,
        Cube* boundingCube);

    void convertToGCC(const Coordinates* c,
                      Coordinates* gccCoord) {
        if (m_terrainData->getCoordinateSystem() == LATLONALT)
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

    void convertToGeodetic(const Coordinates* c,
                           Coordinates* geodeticCoord) {
        if (m_terrainData->getCoordinateSystem() == LATLONALT)
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
    void projectTo2D(const Coordinates* c,
                     Coordinates* projectedCoord) {
        // assume c is geodetic
        Coordinates temp;
        if (m_terrainData->getCoordinateSystem() == LATLONALT)
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
};

#endif /* QUALNET_URBAN_TERRAIN_H */
