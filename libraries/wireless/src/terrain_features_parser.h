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

#ifndef TERRAIN_FEATURES_PARSER_H
#define TERRAIN_FEATURES_PARSER_H

#include "terrain_features.h"

/*! \file terrain_features_parser.h
  \brief Structures and functions used to parse
  Terrain Feature Documents

  This file contains the structures and functions
  that are used to parse terrain features described in
  Qualnet's XML format, into Qualnet's internal structures.
*/

/*! \def TERRAIN_FEATURES_PARSE_BUFF_SIZE
  \brief A constant that defines the size of the buffer
  used to read the XML files
*/
#define TERRAIN_FEATURES_PARSE_BUFF_SIZE 8192

/*! \def TERRAIN_FEATURES_MAX_STACK_DEPTH
  \brief A constant that defines the maximum size of
  the XML object stack
*/
#define TERRAIN_FEATURES_MAX_STACK_DEPTH 10

/*! \def TERRAIN_FEATURES_INIT_ARRAY_SIZE
  \brief A constant that defines the initial array
  size for various temporary arrays
*/
#define TERRAIN_FEATURES_INIT_ARRAY_SIZE 100

/*! \def TERRAIN_FEATURES_INIT_HASH_TABLE_SIZE
  \brief A constant that defines the initial size
  of the hash table used to record XML IDs of the
  XML objects
*/
#define TERRAIN_FEATURES_INIT_HASH_TABLE_SIZE 1000


/*! \enum tags
  \brief XML tags used to identify various XML objects
*/
typedef enum
{
    INVALID_TAG,     /*!< Unrecognized Tag - value 1.*/
    SITE,            /*!< Site Tag - value 2.*/
    REGION,          /*!< Region Tag - value 3.*/
    POSITION,        /*!< position Tag - value 4.*/
    PARK,            /*!< Park Tag - value 5.*/
    REPRESENTATIVE,  /*!< Representative Tag - value 6.*/
    STATION,         /*!< Station Tag - value 7.*/
    BUILDING,        /*!< Building Tag - value 8.*/
    FACE,            /*!< face Tag - value 9.*/
    THICKNESS,       /*!< thickness Tag - value 10.*/
    CLOUD,           /*!< Cloud Tag - value 11.*/
    STREET_SEGMENT,  /*!< Street_Segment Tag - value 12.*/
    FIRST_NODE,      /*!< firstNode Tag - value 13.*/
    NODE,            /*!< node Tag - value 14.*/
    LAST_NODE,       /*!< lastNode Tag - value 15.*/
    WIDTH,           /*!< width Tag - value 16.*/
    RAILROAD,        /*!< Railroad Tag - value 17.*/
    INTERSECTION,    /*!< Intersection Tag - value 18.*/
    LOCATION,        /*!< location Tag - value 19.*/
    SYNC_SIGNAL,     /*!< SynchronizedSignals Tag - value 20.*/
    REFERENCE,       /*!< reference Tag - value 21.*/
    OBJECTS,         /*!< objects Tag - value 22.*/
    FOLIAGE          /*!< Foliage Tag - value 23.*/
} tags;


/*! \enum  HashEntryState
  \brief Indicates whether a hashtable entry is empty or not
*/
typedef enum
{
    LEGITIMATE,     /*!< hashtable entry is Legitimate - value 1.*/
    EMPTY           /*!< hashtable entry is empty - value 2.*/
} HashEntryState;


/*! \enum  HashObjectState
  \brief Indicates whether a hashtable entry has a complete
  or incomplete object
*/
typedef enum
{
    COMPLETE,      /*!< hashtable object is complete - value 1.*/
    INCOMPLETE     /*!< hashtable object is incomplete - value 2.*/
} HashObjectState;


/*! \struct struct_terrain_features_stack
  \brief XML Object Stack used while parsing
*/
/*! \typedef typedef struct TerrainFeaturesStack
  \brief struct_terrain_features_stack is
  typedef'ed to TerrainFeaturesStack
*/
typedef struct struct_terrain_features_stack
{
    tags
    tagStack[TERRAIN_FEATURES_MAX_STACK_DEPTH];      /*!< Stack of XML tags*/
    void*
    objectStack[TERRAIN_FEATURES_MAX_STACK_DEPTH];   /*!< Stack of objects*/
    int depth;                                       /*!< Depth of the stack*/
} TerrainFeaturesStack;


/*! \struct struct_terrain_features_hash_cell
  \brief Contents of an entry of the hashtable
  used to keep track of terrain feature objects
  in terms of their XML IDs
*/
/*! \typedef typedef struct TerrainFeaturesHashCell
  \brief struct struct_terrain_features_hash_cell
  is typedef'ed to TerrainFeaturesHashCell
*/
typedef struct struct_terrain_features_hash_cell
{
    char XML_ID[512];             /*!< XML ID of the object*/
    tags objectType;              /*!< Tag type of the object*/
    int objectID;                 /*!< Object ID*/
    HashEntryState entryState;    /*!< Entry state of the hash cell*/
    HashObjectState objectState;  /*!< Object state of the object*/
} TerrainFeaturesHashCell;


/*! \struct struct_terrain_features_hash_table
  \brief Hashtable used to keep track of
  terrain feature objects in terms of their XML IDs
*/
/*! \typedef typedef struct TerrainFeaturesHashTable
  \brief struct struct_terrain_features_hash_table is
  typedef'ed to TerrainFeaturesHashTable
*/
typedef struct struct_terrain_features_hash_table
{
    int tableSize;                    /*!< Size of the hashtable*/
    int numItems;                     /*!< Number of items in the hashtable*/
    TerrainFeaturesHashCell *cells;   /*!< Cells of the hashtable*/
} TerrainFeaturesHashTable;


/*! \struct struct_terrain_feature_road_links
  \brief Structure used to keep track of the next and previous
  road segments if any for a given road segment
*/
/*! \typedef typedef struct TerrainFeaturesRoadLinks
  \brief struct struct_terrain_feature_road_links is
  typedef'ed to TerrainFeaturesRoadLinks
*/
typedef struct struct_terrain_feature_road_links
{
    RoadSegment* nextRoad;        /*!< Pointer to the next Road Segment*/
    RoadSegment* prevRoad;        /*!< Pointer to the previous Road Segment*/
    BOOL isSecondLast;            /*!< True if the segment is second to last
                                    in a chain of street segments*/
} TerrainFeaturesRoadLinks;



/*! \struct struct_terrain_features_parse_struct
  \brief Structure of temporary structures and variables used
  while parsing XML files for terrain features
*/
/*! \typedef typedef struct TerrainFeaturesParseStruct
  \brief struct struct_terrain_features_parse_struct is
  typedef'ed to TerrainFeaturesParseStruct
*/
typedef struct struct_terrain_features_parse_struct
{
    TerrainFeaturesStack stack;                    /*!< Object Stack*/
    tags prevTag;                                  /*!< Tag of last
                                                     parsed object. It is
                                                     used to check whether
                                                     a sequence of objects
                                                     occur in the specified
                                                     order*/
    TerrainFeaturesHashTable *hashTable;           /*!< Hashtable of objects*/

    //Temp structures and related vars
    FeatureFace *faces;                           /*!< Temporary array of
                                                     building faces*/
    FeatureFace *foliageFaces;                     /*!< Temporary array of
                                                     foliage faces*/

    Coordinates *coordinateList;                   /*!< Temporary array of
                                                     coordinates*/
    RoadSegmentID *roadIDs;                        /*!< Temporary array of
                                                     street segment IDs*/
    IntersectionID *intIDs;                        /*!< Temporary array of
                                                     intersection IDs*/
    int maxFaces;                                  /*!< Size of temporary
                                                     building face array*/
    int maxFeatureFaces;                           /*!< Size of temporary
                                                     foliage face array*/
    int maxCoordinates;                            /*!< Size of temporary
                                                     coordinates array*/
    int maxRoadIds;                                /*!< Size of temporary
                                                     street segment ID
                                                     array*/
    int maxIntIds;                                 /*!< Size of temporary
                                                     intersection ID
                                                     array*/
    int maxStreetSegObjects;                       /*!< Number of
                                                     Street_Segment objects
                                                     in the XML files*/
    int numExtraStreetSegs;                        /*!< Number of extra street
                                                     segments in the files
                                                     caused by placement of
                                                     nodes in the middle of
                                                     street segment
                                                     objects*/
    int numFaces;                                  /*!< Number of faces in the
                                                     temporary building
                                                     face array*/
    int numFeatureFaces;                           /*!< Number of faces in the
                                                     temporary foliage
                                                     face array*/
    int numCoordinates;                            /*!< Number of coordinates
                                                     in the temporary
                                                     coordinates array*/
    int numRoadIds;                                /*!< Number of street
                                                     segment IDs in the
                                                     temporary array*/

    TrafficLight* trafficLights;
    int numTrafficLights;
    int maxTrafficLights;
    int numSignalGroups;

    //Reference coordinate structures
    Coordinates globalReference;                   /*!< Global reference
                                                     coordinates*/
    CoordinateRepresentationType globalType;       /*!< Global coordinate
                                                     type*/
    Coordinates localReference;                    /*!< Local reference
                                                     coordinates*/
    CoordinateRepresentationType localType;        /*!< Local coordinate
                                                     type*/
    Coordinates buildingReference;                 /*!< Building reference
                                                     coordinates*/
    CoordinateRepresentationType buildingType;     /*!< Building coordinates
                                                     type*/
    Coordinates foliageReference;                  /*!< Foliage reference
                                                     coordinates*/
    CoordinateRepresentationType foliageType;      /*!< Foliage coordinates
                                                     type*/

    //Temporary character buffers
    char *siteID;                                  /*!< String containing
                                                     XML ID of the site
                                                     object*/
    char dataBuf[512];                             /*!< Temporary data
                                                     buffer*/

    //Variables used to count objects during the first pass
    int maxRoadSegments;                           /*!< Number of street
                                                     segment Objects
                                                     to be parsed*/
    int maxBuildings;                              /*!< Number of building
                                                     objects to be parsed*/
    int maxFoliage;                                /*!< Number of foliage
                                                     objects to be parsed*/
    int maxIntersections;                          /*!< Number of
                                                     intersections to be
                                                     parsed*/
    int maxParks;                                  /*!< Number of parks to
                                                     be parsed*/
    int maxStations;                               /*!< Number of stations to
                                                     be parsed*/
    int maxClouds;                                 /*!< Number of clouds to
                                                     be parsed*/
    int maxRailroads;                              /*!< Number of railroads
                                                     to be parsed*/

    //Coordinate System type specified in the config file
    int coordinateSystemType;                      /*!< Coordinate system type
                                                     specified in the
                                                     qualnet config file*/

    //Pointers to data structures where parsed objects are stored
    TerrainFeaturesData *terrainFeatures;          /*!< Pointer to the
                                                     TerrainFeaturesData
                                                     structure*/
    TerrainData *terrainData;                     /*!< Pointer to the
                                                    TerrainData
                                                    structure*/
    BOOL southWestSet;                            /*!< Boolean variable to
                                                    indicate whether the
                                                    south west coordinate
                                                    has been set yet*/
    BOOL globalSet;                                /*!< Boolean variable to
                                                     indicate whether the global
                                                     reference coordinates and
                                                     type have been sen*/

    BOOL subtractTerrain;
} TerrainFeaturesParseStruct;

#endif //TERRAIN_FEATURES_H
