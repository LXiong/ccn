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

#ifndef TERRAIN_FEATURES_H
#define TERRAIN_FEATURES_H

#include "api.h"
#include "simplesplay.h"

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

#define DEFAULT_SEPARATION_SUM 1.0

#define DEFAULT_SEPARATION_AVG 1.0

typedef struct
{
    CoordinateType minX;
    CoordinateType maxX;
    CoordinateType minY;
    CoordinateType maxY;
    CoordinateType minZ;
    CoordinateType maxZ;
} DimensionRange;

typedef struct
{
    double normalX;
    double normalY;
    double normalZ;
    double d;
} PlaneParameters;

typedef struct
{
    char* XML_ID;
    int num_vertices;
    Coordinates* vertices;
    double thickness;
    DimensionRange properties;
    PlaneParameters plane;
    // void* buildingFaceVar;
} FeatureFace;

typedef struct
{
  int numBoxes_BuildingBelongsTo;
  int* listBoxes_BuildingBelongsTo;
  int gridBox_WithBuildingData;
  int BldgIdxIn_gridBox_WithBuildingData;
} FeatureGridInfo;

typedef struct
{
    char* XML_ID;
    char* featureName;
    int num_faces;
    FeatureFace* faces;
    DimensionRange properties;
    double height;
    double density;
    // void* buildingVar;
    FeatureGridInfo* featureGridInfo;
} Building;

typedef Building Foliage;

typedef int RoadSegmentID;
typedef int IntersectionID;
typedef int RoadID;
typedef int StationID;
typedef int ParkID;
typedef int BuildingID;
typedef int FoliageID;

typedef int ParkStationID;

typedef int FaceIndex;

#define FACE_RECEIVER -1
#define FACE_TRANSMITTER -2

typedef struct //struct_road_segment
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
} RoadSegment;

typedef enum
{
    ENTRANCE_NONE = 0,
    ENTRANCE_STATION,
    ENTRANCE_PARK
} EntranceType;

typedef int SignalGroupNumber;

typedef struct //struct_intersection
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
    BOOL hasTrafficSignal;
    int numSignalGroups;
    SignalGroupNumber* signalGroups;

    void* intersectionVar;
} Intersection;

// used to fill dummy in Intersection during parsing of XML
typedef struct
{
    SignalGroupNumber signalGroup;
    RoadSegmentID roadID;
} TrafficLight;

typedef struct
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
} Station;

typedef struct
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
} ParkStation;

typedef ParkStation Park;

typedef struct struct_terrain_features_data
{
    Coordinates southWest;          /*!< Coordinates of south-western
                                      most point*/
    Building *buildings;            /*!< Array of buildings parsed*/
    Foliage *foliage;               /*!< Array of foliage parsed*/
    RoadSegment *roadSegments;      /*!< Array of road segments parsed*/
    Intersection *intersections;    /*!< Array of intersections parsed*/
    Park *parks;                    /*!< Array of Parks parsed*/
    Station *stations;              /*!< Array of Stations parsed*/
    int numIntersections;           /*!< Number of intersections parsed*/
    int numRoadSegments;            /*!< Number of road segments parsed*/
    int numBuildings;               /*!< Number of buildings parsed*/
    int numFoliage;                 /*!< Number of foliage parsed*/
    int numParks;                   /*!< Number of parks parsed*/
    int numStations;                /*!< Number of stations parsed*/
    int numClouds;                  /*!< Number of clouds parsed*/
    int numRailroads;               /*!< Number of railroads parsed*/

    float totalBuildingHeight;
    float minBuildingHeight;
    float maxBuildingHeight;

    float totalFoliageHeight;
    float minFoliageHeight;
    float maxFoliageHeight;
    FoliatedState foliatedState;

    void* roadSegmentVar;           /*!< Array for dynamic data for roads*/
    size_t roadElementSize;
    void* parkStationVar;           /*!< Array of dynamic data for parks*/
    size_t parkStationElementSize;
    SimpleSplayTree pedestrianSplayTree;

    BOOL setFeaturesToGround;
} TerrainFeaturesData;

// functions follow

void ENVIRONMENT_GetFresnelPathParameters(
    TerrainData* terrainData,
    Building* features,
    int numFeatures,
    const Coordinates* source,
    const Coordinates* dest,
    double diameter,
    int* const num,
    double** const lengths,
    double** const densities,
    double* const avgBuildingHeight,
    short* const orientation,
    double* const avgBuildingSeparation,
    double* const txDistanceToBuilding,
    double* const rxDistanceToBuilding,
    double* const buildingCrossSection);

/*
void ENVIRONMENT_GetDistanceFromCorner(
    TerrainData* terrainData,
    const Coordinates* source,
    const Coordinates* dest,
    BuildingID building,
    FaceIndex faces[2],
    BOOL* const haveCorner,
    double* const txDistance,
    double* const rxDistance);
*/

void ENVIRONMENT_ReturnIntersectionBuildings(
    TerrainData* terrainData,
    Building* features,
    int numFeatures,
    const Coordinates* source,
    const Coordinates* dest,
    int* const numResults,
    BuildingID** const buildings,
    double** const overlappingDistances,
    Coordinates (** const intersections)[2],
    FaceIndex (** const faces)[2]);

BOOL ENVIRONMENT_CalculateOverlap(
    int coordinateSystemType,
    const Coordinates* source,
    const Coordinates* dest,
    const Building* obstruction,
    Coordinates* const intersection1,
    Coordinates* const intersection2,
    FaceIndex* const face1,
    FaceIndex* const face2);

BOOL ENVIRONMENT_CalculateIntersectionSegmentPlane(
    int coordinateSystemType,
    const Coordinates* source,
    const Coordinates* dest,
    const FeatureFace* face,
    Coordinates* const intersection);

BOOL ENVIRONMENT_PassThroughTest(
    const Coordinates* source,
    const Coordinates* dest,
    const DimensionRange* properties);

void ENVIRONMENT_CreateDimensionRange(Building* building);

void ENVIRONMENT_CreatePlaneParameters(Building* building);

void ENVIRONMENT_CalculateIntersection3Planes(
        const FeatureFace* face1,
        const FeatureFace* face2,
        const FeatureFace* face3,
        Coordinates* const intersection);

#define COSINE_ERROR_FIRST_SECOND -3
#define COSINE_ERROR_FIRST_THIRD -2
#define COSINE_ERROR_ALL -1

double ENVIRONMENT_CosineOfVectors(
    int coordinateSystemType,
    const Coordinates* source,
    const Coordinates* dest1,
    const Coordinates* dest2);

//typedef struct struct_terrain_data_str TerrainData;

double ENVIRONMENT_CalcDistanceOverTerrain(
    TerrainData* terrainData,
    const Coordinates* position1,
    const Coordinates* position2,
    double elevationSamplingDistance);


void ENVIRONMENT_RoadGiveIntermediatePosition(
    Coordinates* const position,
    RoadSegmentID ID,
    IntersectionID startIntersection,
    double traveling_distance,
    TerrainData* terrainData);

//
// inlined functions
//
static //inline//
int ENVIRONMENT_NumberOfParks(TerrainData* terrainData)
{
    return terrainData->terrainFeatures->numParks;
}

static //inline//
int ENVIRONMENT_NumberOfStations(TerrainData* terrainData)
{
    return terrainData->terrainFeatures->numStations;
}

static //inline//
int ENVIRONMENT_NumberOfIntersections(TerrainData* terrainData)
{
    return terrainData->terrainFeatures->numIntersections;
}

static //inline//
int ENVIRONMENT_NumberOfRoadSegments(TerrainData* terrainData)
{
    return terrainData->terrainFeatures->numRoadSegments;
}

//
// RoadSegment functions
//

static //inline//
RoadSegment* ENVIRONMENT_RoadSegment(
    RoadSegmentID ID,
    TerrainData* terrainData)
{
    return &(terrainData->terrainFeatures->roadSegments[ID]);
}

static //inline//
double ENVIRONMENT_RoadSegmentArea(
    RoadSegmentID ID,
    TerrainData* terrainData)
{
    RoadSegment* rs = ENVIRONMENT_RoadSegment(ID, terrainData);
    return rs->width * rs->length;
}

static //inline//
double ENVIRONMENT_RoadSegmentWidth(
    RoadSegmentID ID,
    TerrainData* terrainData)
{
    RoadSegment* rs = ENVIRONMENT_RoadSegment(ID, terrainData);
    return rs->width;
}

static //inline//
double ENVIRONMENT_RoadSegmentLength(
    RoadSegmentID ID,
    TerrainData* terrainData)
{

    RoadSegment* rs = ENVIRONMENT_RoadSegment(ID, terrainData);
    return rs->length;
}

static //inline//
void ENVIRONMENT_AllocateAllRoadSegmentVars(
    size_t numBytes,
    TerrainData* terrainData)
{
    terrainData->terrainFeatures->roadSegmentVar =
        MEM_malloc(terrainData->terrainFeatures->numRoadSegments
                   * numBytes);

    terrainData->terrainFeatures->roadElementSize = numBytes;
}

static //inline//
void* ENVIRONMENT_ReturnRoadSegmentVar(
    RoadSegmentID ID,
    TerrainData* terrainData)
{
    if (terrainData->terrainFeatures->roadSegmentVar == NULL)
    {
        ERROR_ReportWarning(
            "ENVIRONMENT_ReturnRoadSegmentVar: User data has not yet "
            "been allocated.\n");
    }

    // assumes that char are one byte
    return (char*) terrainData->terrainFeatures->roadSegmentVar
            + (terrainData->terrainFeatures->roadElementSize * ID);
}

static //inline//
void ENVIRONMENT_FreeRoadSegmentVar(
    TerrainData* terrainData)
{
    if (terrainData->terrainFeatures->roadSegmentVar != NULL)
    {
        MEM_free(terrainData->terrainFeatures->roadSegmentVar);
        terrainData->terrainFeatures->roadSegmentVar = NULL;
    }
    else
    {
        ERROR_ReportWarning(
            "ENVIRONMENT_FreeRoadSegmentVar: User data has not "
            "been allocated.\n");
    }
}

static //inline//
void ENVIRONMENT_IntersectionsOfRoadSegment(
    RoadSegmentID ID,
    IntersectionID* firstIntersection,
    IntersectionID* secondIntersection,
    TerrainData* terrainData)
{
    RoadSegment* rs = ENVIRONMENT_RoadSegment(ID, terrainData);
    *firstIntersection = rs->firstIntersection;
    *secondIntersection = rs->secondIntersection;
}

// sets points to point to location of vertices where they are
// actually stored for the terraindata
static //inline//
void ENVIRONMENT_RoadSegmentPoints(
    RoadSegmentID ID,
    const Coordinates** points,
    int* const num_points,
    TerrainData* terrainData)
{
    RoadSegment* rs = ENVIRONMENT_RoadSegment(ID, terrainData);
    *points = (const Coordinates*) rs->vertices;
    *num_points = rs->num_vertices;
}

//
// Intersection functions
//

static //inline//
Intersection* ENVIRONMENT_Intersection(
    IntersectionID ID,
    TerrainData* terrainData)
{
    return &(terrainData->terrainFeatures->intersections[ID]);
}

// sets roadSegments to point to location of adjacent road segments
// where they are actually stored for the terraindata
static //inline//
void ENVIRONMENT_AdjacentRoadSegments(
    IntersectionID ID,
    const RoadSegmentID** roadSegments,
    int* num_road_segments,
    TerrainData* terrainData)
{
    Intersection* i = ENVIRONMENT_Intersection(ID, terrainData);
    *roadSegments = (const RoadSegmentID*) i->roadSegments;
    *num_road_segments = i->num_road_segments;
}

// sets neighbors to point to location of adjacent intersection
// where they are actually stored for the terraindata
static //inline//
void ENVIRONMENT_NeighboringIntersections(
    IntersectionID ID,
    const IntersectionID** neighbors,
    int* num_neighbors,
    TerrainData* terrainData)
{
    Intersection* i = ENVIRONMENT_Intersection(ID, terrainData);
    *neighbors = (const IntersectionID*) i->neighbors;
    *num_neighbors = i->num_road_segments;
}

static //inline//
void ENVIRONMENT_TrafficSignalGroups(
    IntersectionID ID,
    const SignalGroupNumber** signalGroups,
    int* num_neighbors,
    TerrainData* terrainData)
{
    Intersection* i = ENVIRONMENT_Intersection(ID, terrainData);
    *signalGroups = (const IntersectionID*) i->signalGroups;
    *num_neighbors = i->num_road_segments;
}

static //inline//
EntranceType ENVIRONMENT_IntersectionEntranceType(
    IntersectionID ID, TerrainData* terrainData)
{
    Intersection* i = ENVIRONMENT_Intersection(ID, terrainData);
    return i->entranceType;
}

static //inline//
int ENVIRONMENT_NumberOfNeighboringIntersections(
    IntersectionID ID, TerrainData* terrainData)
{
    Intersection* i = ENVIRONMENT_Intersection(ID, terrainData);
    return i->num_road_segments;
}

// returns const* to coordinates of intersection
static //inline//
const Coordinates* ENVIRONMENT_IntersectionLocation(
    IntersectionID ID,
    TerrainData* terrainData)
{
    Intersection* i = ENVIRONMENT_Intersection(ID, terrainData);
    return (const Coordinates*) &(i->position);
}

#if 0
// copies coordinates of intersection to location specified
static //inline//
void ENVIRONMENT_IntersectionLocation(
    IntersectionID ID,
    Coordinates* save)
{
    Intersection* i = ENVIRONMENT_Intersection(ID);
    memcpy(save, &(i->position), sizeof(Coordinates));
    #if 0
    save->common.c1 = i->position.common.c1;
    save->common.c2 = i->position.common.c2;
    save->common.c3 = i->position.common.c3;
    save->type = i->position.type;
    #endif
}
#endif

static //inline//
ParkID ENVIRONMENT_IntersectionPark(
    IntersectionID ID,
    TerrainData* terrainData)
{
    Intersection* i = ENVIRONMENT_Intersection(ID, terrainData);
    return i->park;
}

static //inline//
BOOL ENVIRONMENT_IntersectionHasTrafficSignal(
    IntersectionID ID,
    TerrainData* terrainData)
{
    Intersection* i = ENVIRONMENT_Intersection(ID, terrainData);
    return i->hasTrafficSignal;
}

// ParkStationFunctions

static //inline//
ParkStation* ENVIRONMENT_ParkStation(
    ParkStationID ID,
    TerrainData* terrainData)
{
    return &(terrainData->terrainFeatures->parks[ID]);
}

static //inline//
void ENVIRONMENT_AllocateAllParkStationVars(
    size_t numBytes,
    TerrainData* terrainData)
{
    terrainData->terrainFeatures->parkStationVar =
        MEM_malloc(terrainData->terrainFeatures->numParks
                   * numBytes);

    terrainData->terrainFeatures->parkStationElementSize = numBytes;
}

static //inline//
void* ENVIRONMENT_ReturnParkStationVar(
    ParkStationID ID,
    TerrainData* terrainData)
{
    if (terrainData->terrainFeatures->parkStationVar == NULL)
    {
        ERROR_ReportWarning(
            "ENVIRONMENT_ReturnParkStationVar: User data has not yet "
            "been allocated.\n");
    }

    // assumes that char are one byte
    return (char*) terrainData->terrainFeatures->parkStationVar
            + (terrainData->terrainFeatures->parkStationElementSize * ID);
}

static //inline//
void ENVIRONMENT_FreeParkStationVar(
    TerrainData* terrainData)
{
    if (terrainData->terrainFeatures->parkStationVar != NULL)
    {
        MEM_free(terrainData->terrainFeatures->parkStationVar);
        terrainData->terrainFeatures->parkStationVar = NULL;
    }
    else
    {
        ERROR_ReportWarning(
            "ENVIRONMENT_FreeParkStationVar: User data has not "
            "been allocated.\n");
    }
}

// sets points to point to location of vertices where they are
// actually stored for the terraindata
static //inline//
void ENVIRONMENT_ParkStationPoints(
    ParkStationID ID,
    const Coordinates** points,
    int* const num_points,
    TerrainData* terrainData)
{
    ParkStation* ps = ENVIRONMENT_ParkStation(ID, terrainData);
    *points = (const Coordinates*) ps->vertices;
    *num_points = ps->num_vertices;
}


static //inline//
void ENVIRONMENT_ParkStationEntrances(
    ParkStationID ID,
    const IntersectionID** exits,
    int* const num_points,
    TerrainData* terrainData)
{
    ParkStation* ps = ENVIRONMENT_ParkStation(ID, terrainData);
    *exits = (const IntersectionID*) ps->exitIntersections;
    *num_points = ps->num_vertices;
}

static //inline//
const Coordinates* ENVIRONMENT_ParkStationRepresentativeCoordinate(
    ParkStationID ID,
    TerrainData* terrainData)
{
    ParkStation* ps = ENVIRONMENT_ParkStation(ID, terrainData);
    return (const Coordinates*) &(ps->representative);
}

static //inline//
double ENVIRONMENT_GetAvgHRoof(TerrainData* terrainData)
{
    if (terrainData->terrainFeatures == NULL)
        return 0.0;
    else
        return terrainData->terrainFeatures->totalBuildingHeight
            / terrainData->terrainFeatures->numBuildings;
}

static //inline//
double ENVIRONMENT_GetMaxBuildingHeight(TerrainData* terrainData)
{
    if (terrainData->terrainFeatures == NULL)
        return 0.0;
    else
        return terrainData->terrainFeatures->maxBuildingHeight;
}

static //inline//
double ENVIRONMENT_GetMinBuildingHeight(TerrainData* terrainData)
{
    if (terrainData->terrainFeatures == NULL)
        return 0.0;
    else
        return terrainData->terrainFeatures->minBuildingHeight;
}

// inside or outside function, for general use

// initialize grid of custom size objects, determine each inside or not

// parkstation grid ij

// location transformation

// in terrain_features_parser.cpp, with data types and #define's in
// terrain_features_parser.h

/*! \fn void MOBILITY_TerrainFeaturesInitialize(TerrainData*
  &terrainData,
  NodeInput *nodeInput);
  \brief Function called to parse terrain feature data from the
  XML files
  \param terrainData pointer to a TerrainData structure
  \param nodeInput Pointer to the qualnet config input structure

  This function is called during initialization of terrain data
  and is used to parse all terrain feature data specified in the XML files.
  The names of these XML files and other config parameters are specified
  in the qualnet config file.
*/
void MOBILITY_TerrainFeaturesInitialize(TerrainData *terrainData,
                                        NodeInput *nodeInput,
                                        int thisPartitionId);

/*! \fn void TERRAIN_UrbanParameterList(NodeInput *nodeInput);
  \brief Returns the urban terrain parameters in a string
  \param nodeInput Pointer to the qualnet config input structure

  This function is called during initialization to return a list of
  the urban files and associated parameters for the GUI.
*/
char* TERRAIN_UrbanParameterList(NodeInput *nodeInput);

/*! \fn void MOBILITY_TerrainFeaturesFinalize(TerrainFeaturesData*
  terrainFeaturesData);
  \brief Function called to free all structures used to store
  terrain feature data
  \param terrainFeaturesData pointer to a TerrainFeaturesData structure

  This function is called during while finalizing all structures at the
  end of the simulation.
*/
void MOBILITY_TerrainFeaturesFinalize(TerrainFeaturesData* terrainFeaturesData);

void ENVIRONMENT_GroupSignals(TerrainData* terrainData);

double ENVIRONMENT_CalculateBuildingHeight(const Building* building);

void ENVIRONMENT_AddHeightInformation(
    Building* building,
    float* minBuildingHeight,
    float* maxBuildingHeight,
    float* totalBuildingHeight);

void ENVIRONMENT_ConvertBuildingCoordinates(
        Building* building,
        CoordinateRepresentationType type);

void ENVIRONMENT_SetTerrainFeaturesToGround(
    TerrainData* terrainData,
    Building* features,
    int numFeatures);

#endif /* TERRAIN_FEATURES_H */
