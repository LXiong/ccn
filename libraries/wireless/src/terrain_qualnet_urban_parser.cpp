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
#include <expat.h>
#include <string.h>
#include <limits.h>

#ifdef USE_MPI
#include <mpi.h>
#endif

#include "qualnet_error.h"
#include "random.h"
#include "terrain_qualnet_urban_parser.h"

#ifdef CTDB7_INTERFACE
#include "ctdb_7_converter.h"
#endif // CTDB7_INTERFACE

#ifdef CTDB8_INTERFACE
#include "ctdb_8_converter.h"
#endif // CTDB8_INTERFACE

//#ifdef ESRI_SHP_INTERFACE
#include "terrain_esri_shp.h"
//#endif // ESRI_SHP_INTERFACE

#define DEBUG 0

// static function declarations

static
int GetNextPrime(int n);

static
unsigned int Hash(TerrainFeaturesParseStruct *parse,
                  const char *key);

static
void InitializeTable(TerrainFeaturesParseStruct *parse,
                     int tableSize);

static
int Find(TerrainFeaturesParseStruct *parse,
         const char* key);

static
int InsertObject(TerrainFeaturesParseStruct *parse,
                 const char* key,
                 int objectID,
                 tags objectType);

static
void DestroyTable(TerrainFeaturesParseStruct *parse);

static
void XMLCALL InitStartTag(void *data,
                          const char *el,
                          const char **attr);


static
void XMLCALL InitEndTag(void *data,
                        const char *el);

static
void parseCoordinates(TerrainFeaturesParseStruct *parse,
                      Coordinates &coordinates);

static
void parseCoordinatesBuilding(TerrainFeaturesParseStruct *parse,
                              Coordinates &coordinates);

static
void XMLCALL HandleStartTag(void *data,
                            const char *el,
                            const char **attr);

static
void XMLCALL HandleEndTag(void *data,
                          const char *el);

static
void XMLCALL HandleData(void *userData,
                        const XML_Char *s,
                        int len);

// static functions

/*! \fn static int GetNextPrime(int n)
  \brief Returns the smallest prime number
  greater that n
  \param n integer input
  \return smallest prime number greater than n
*/
static
int GetNextPrime(int n)
{
    int i;
    int found;

    if (n % 2 == 0) {
        n++;
    }
    for (; ; n += 2)
    {
        found = 1;
        for (i = 3; i*i <= n; i += 2)
        {
            if (n % i == 0 ) {
                found = 0;
                break;
            }
        }
        if (found == 1) {
            return n;
        }
    }
}

/*! \fn static
  unsigned int Hash(TerrainFeaturesParseStruct *parse,
  const char *key)
  \brief Hashes the the key string
  \param parse pointer to the TerrainFeaturesParseStruct structure
  \param key the key string
  \return The hash value
*/
static
unsigned int Hash(TerrainFeaturesParseStruct *parse,
                  const char *key)
{
    unsigned int hashVal = 0;

    while (*key != '\0' )
        hashVal += *key++;

    return hashVal % parse->hashTable->tableSize;
}

/*! \fn static
  void InitializeTable(TerrainFeaturesParseStruct *parse,
  int tableSize)
  \brief Function called to initializes the Hash Table
  \param parse pointer to the TerrainFeaturesParseStruct structure
  \param tableSize The minimum Hash Table size
*/
static
void InitializeTable(TerrainFeaturesParseStruct *parse,
                     int tableSize)
{
    int i;

    parse->hashTable = (TerrainFeaturesHashTable *)
                       MEM_malloc(sizeof(TerrainFeaturesHashTable));
    parse->hashTable->tableSize = GetNextPrime(tableSize);
    parse->hashTable->cells = (TerrainFeaturesHashCell *)
                              MEM_malloc(sizeof(TerrainFeaturesHashCell) *
                                         parse->hashTable->tableSize);
    parse->hashTable->numItems = 0;

    for (i = 0; i < parse->hashTable->tableSize; i++) {
        parse->hashTable->cells[i].entryState = EMPTY;
        parse->hashTable->cells[i].objectState = INCOMPLETE;
        parse->hashTable->cells[i].objectID = -1;
        parse->hashTable->cells[i].objectType = INVALID_TAG;
    }
}

/*! \fn static
  int Find(TerrainFeaturesParseStruct *parse,
  const char* key)
  \brief Function called to find the position of
  the cell corresponding to a given key
  \param parse pointer to the TerrainFeaturesParseStruct structure
  \param key the key string
  \return The index into the hash table corresponding to the key
  string
*/
static
int Find(TerrainFeaturesParseStruct *parse,
         const char* key)
{
    int currentPos;
    int collisionNum;

    collisionNum = 0;
    currentPos = Hash(parse,
                      key);
    while ((parse->hashTable->cells[currentPos].
            entryState == LEGITIMATE) &&
            (strcmp(parse->hashTable->cells[currentPos].
                    XML_ID, key) != 0))
    {
        collisionNum = collisionNum + 1;
        currentPos = currentPos + 2*collisionNum - 1;
        if (currentPos >= parse->hashTable->tableSize) {
            currentPos -= parse->hashTable->tableSize;
        }
    }
    return currentPos;
}

/*! \fn static
  int InsertObject(TerrainFeaturesParseStruct *parse,
  const char* key,
  int objectID,
  tags objectType)
  \brief Function called to insert an object into the Hash Table
  \param parse pointer to the TerrainFeaturesParseStruct structure
  \param key the key string
  \param objectID the object ID of the inserted object
  \param objectType the type of the object inserted
  \return The index of the inserted object in the hash table
*/
static
int InsertObject(TerrainFeaturesParseStruct *parse,
                 const char* key,
                 int objectID,
                 tags objectType) {
    int retVal = 1;
    int position;
    int i;
    int oldSize;
    int insertSuccess;
    TerrainFeaturesHashCell *oldCells;

    //If hashtable is more that half full, resize it
    if (parse->hashTable->numItems > parse->hashTable->tableSize/2)
    {
        if (DEBUG) {
            printf( "Rehashing...\n" );
        }
        oldCells = parse->hashTable->cells;
        oldSize  =  parse->hashTable->tableSize;

        MEM_free(parse->hashTable);
        InitializeTable(parse, 2 * oldSize);

        for (i = 0; i < oldSize; i++) {
            if (oldCells[i].entryState == LEGITIMATE) {
                insertSuccess = InsertObject(parse,
                                             oldCells[i].XML_ID,
                                             oldCells[i].objectID,
                                             oldCells[i].objectType);
                if (!insertSuccess) {
                    ERROR_ReportError("Error during Rehash");

                }
            }
        }
        MEM_free(oldCells);
    }

    //Insert Object
    position = Find(parse, key);
    if (parse->hashTable->cells[position].entryState == EMPTY)
    {
        parse->hashTable->cells[position].entryState = LEGITIMATE;
        strcpy(parse->hashTable->cells[position].XML_ID, key);
        parse->hashTable->cells[position].objectID = objectID;
        parse->hashTable->cells[position].objectType = objectType;
        parse->hashTable->cells[position].objectState = INCOMPLETE;
        parse->hashTable->numItems++;
    }
    else if (objectType == parse->hashTable->cells[position].objectType) {
        retVal = 0;
    }
    else {
        ERROR_ReportError("Same tag used for different objects");

    }
    return retVal;
}

/*! \fn static
  void DestroyTable(TerrainFeaturesParseStruct *parse)
  \brief Function called free the hash table structure
  \param parse pointer to the TerrainFeaturesParseStruct structure
*/
static
void DestroyTable(TerrainFeaturesParseStruct *parse)
{
    MEM_free(parse->hashTable->cells);
    MEM_free(parse->hashTable);
}


/*! \fn static
  void XMLCALL InitStartTag(void *data,
  const char *el,
  const char **attr)
  \brief Start tag handler called by expat during first pass
  over the XML files - this pass is used to fill the hash table
  and count the number of objects of different types
  \param data pointer to the TerrainFeaturesParseStruct structure
  \param el start tag string
  \param attr array of attribute strings for the XML object
  being parsed
*/
static
void XMLCALL InitStartTag(void *data,
                          const char *el,
                          const char **attr)
{
    TerrainFeaturesParseStruct *parse = (TerrainFeaturesParseStruct *)data;
    int i;
    int insertionOccured;
    BOOL siteIDFlag = FALSE;
    BOOL siteRefCoordFlag = FALSE;
    BOOL siteCoordSysFlag = FALSE;

    //Add any relevant object, such as intersection,
    //building, Park, etc to the hash table
    if (strcmp(el, "Site") == 0) {
        //printf("Site\n");
        for (i = 0; attr[i]; i += 2) {
            if (strcmp(attr[i], "id") == 0) {
                if (parse->siteID == NULL ) {
                    parse->siteID = (char *)
                                    MEM_malloc(sizeof(char) * (strlen(attr[i+1]) + 1));
                    strcpy(parse->siteID, attr[i+1]);
                }
                else if (strcmp(parse->siteID, attr[i+1])
                         != 0) {
                    ERROR_ReportError("Different Site IDs accross files");
                }
                siteIDFlag = TRUE;
            }
            else if (strcmp(attr[i], "ReferencePoint") == 0) {
                siteRefCoordFlag = TRUE;
            }
            else if (strcmp(attr[i], "CoordinateType") == 0) {
                siteCoordSysFlag = TRUE;
            }
        }
        if ((siteIDFlag == FALSE) ||
                (siteRefCoordFlag == FALSE) ||
                (siteCoordSysFlag == FALSE))
        {
            ERROR_ReportError("Terrain file missing Site attribute(s)");
        }
    }
    else if (strcmp(el, "Park") == 0) {
        //printf("Park\n");
        for (i = 0; attr[i]; i += 2) {
            if (strcmp(attr[i], "id") == 0) {
                insertionOccured = InsertObject(parse,
                                                attr[i+1],
                                                parse->maxParks,
                                                PARK);
                if (insertionOccured) {
                    parse->maxParks++;
                }
            }
        }
    }
    else if (strcmp(el, "Station") == 0) {
        //printf("Station\n");
        for (i = 0; attr[i]; i += 2) {
            if (strcmp(attr[i], "id") == 0) {
                insertionOccured = InsertObject(parse,
                                                attr[i+1],
                                                parse->maxStations,
                                                STATION);
                if (insertionOccured) {
                    parse->maxStations++;
                }
            }
        }
    }
    else if (strcmp(el, "Building") == 0) {
        //printf("Building\n");
        for (i = 0; attr[i]; i += 2) {
            if (strcmp(attr[i], "id") == 0) {
                insertionOccured = InsertObject(parse,
                                                attr[i+1],
                                                parse->maxBuildings,
                                                BUILDING);
                if (insertionOccured) {
                    parse->maxBuildings++;
                }
            }
        }
    }
    else if (strcmp(el, "Foliage") == 0) {
        //printf("Foliage\n");
        for (i = 0; attr[i]; i += 2) {
            if (strcmp(attr[i], "id") == 0) {
                insertionOccured = InsertObject(parse,
                                                attr[i+1],
                                                parse->maxFoliage,
                                                FOLIAGE);
                if (insertionOccured) {
                    parse->maxFoliage++;
                }
            }
        }
    }
    else if (strcmp(el, "Cloud") == 0) {
        //printf("Cloud\n");
        for (i = 0; attr[i]; i += 2) {
            if (strcmp(attr[i], "id") == 0) {
                insertionOccured = InsertObject(parse,
                                                attr[i+1],
                                                parse->maxClouds,
                                                CLOUD);
                if (insertionOccured) {
                    parse->maxClouds++;
                }
            }
        }
    }
    else if (strcmp(el, "Street_Segment") == 0) {
        //printf("Street_Segment\n");
        for (i = 0; attr[i]; i += 2) {
            if (strcmp(attr[i], "id") == 0) {
                insertionOccured = InsertObject(
                                       parse,
                                       attr[i+1],
                                       parse->maxStreetSegObjects,
                                       STREET_SEGMENT);
                if (insertionOccured) {
                    parse->maxRoadSegments++;
                    parse->maxStreetSegObjects++;
                }
            }
        }
        parse->stack.tagStack[parse->stack.depth] = STREET_SEGMENT;
        parse->stack.depth++;

    }
    else if (strcmp(el, "firstNode") == 0) {
        //printf("firstNode\n");
        if (!attr[0] || (strcmp(attr[0], "objectRef") != 0)) {
            ERROR_ReportError("lastNode tag missing objectRef attribute");
        }
    }
    else if (strcmp(el, "node") == 0) {
        //printf("node\n");
        if (attr[0] && (strcmp(attr[0], "objectRef") == 0)) {
            //break road and take care of all that stuff
            if ((parse->stack.depth == 1) &&
                    (parse->stack.
                     tagStack[parse->stack.depth - 1]
                     == STREET_SEGMENT))
            {
                parse->maxRoadSegments++;
            }
            else {
                ERROR_ReportError("Node objects with intersection Ids "
                                  "is currently not allowed");
            }
        }
    }
    else if (strcmp(el, "lastNode") == 0) {
        //printf("lastNode\n");
        if (!attr[0] || (strcmp(attr[0], "objectRef") != 0)) {
            ERROR_ReportError("lastNode tag missing objectRef attribute");
        }
    }
    else if (strcmp(el, "Railroad") == 0) {
        //printf("Railroad\n");
        for (i = 0; attr[i]; i += 2) {
            if (strcmp(attr[i], "id") == 0) {
                insertionOccured = InsertObject(parse,
                                                attr[i+1],
                                                parse->maxRailroads,
                                                RAILROAD);
                if (insertionOccured) {
                    parse->maxRailroads++;
                }
            }
        }
    }
    else if (strcmp(el, "Intersection") == 0) {
        //printf("Intersection\n");
        for (i = 0; attr[i]; i += 2) {
            if (strcmp(attr[i], "id") == 0) {
                insertionOccured = InsertObject(
                                       parse,
                                       attr[i+1],
                                       parse->maxIntersections,
                                       INTERSECTION);
                if (insertionOccured) {
                    parse->maxIntersections++;
                }
            }
        }
    }
}

/*! \fn static
  void XMLCALL InitEndTag(void *data,
  const char *el)
  \brief End tag handler called by expat during first pass
  over the XML files
  \param data pointer to the TerrainFeaturesParseStruct structure
  \param el end tag string
*/
static
void XMLCALL InitEndTag(void *data,
                        const char *el)
{
    TerrainFeaturesParseStruct *parse = (TerrainFeaturesParseStruct *)data;

    if (strcmp(el, "Street_Segment") == 0)
    {
        if (parse->stack.depth == 1) {
            parse->stack.depth--;
        }
        else if (parse->stack.depth != 0) {
            ERROR_ReportError("Malformed XML file");
        }
    }
}

/*! \fn static
  void parseCoordinates(TerrainFeaturesParseStruct *parse,
  Coordinates &coordinates)
  \brief Function called to parse coordinates from the string buffer
  into the relevant structure
  \param parse pointer to the TerrainFeaturesParseStruct structure
  \param coordinates reference to Coordinates structure into which
  the coordinates are to be parsed
*/
static
void parseCoordinates(TerrainFeaturesParseStruct *parse,
                      Coordinates &coordinates)
{
    Coordinates source;
    Coordinates* ref;
    double elevation;

    //If coordinates are of type "cartesian",
    //read in the coordinates and convert the coordinates
    //to geodetic type using the relevant reference coordinates
    if ((parse->localType == JGIS) ||
            ((parse->localType ==
              INVALID_COORDINATE_TYPE) &&
             (parse->globalType == JGIS)))
    {
        sscanf(parse->dataBuf, "%lf %lf %lf",
               &(source.cartesian.x),
               &(source.cartesian.y),
               &(source.cartesian.z));
        source.type = JGIS;

        if (parse->coordinateSystemType == LATLONALT) {
            if (parse->localReference.latlonalt.latitude == 360) {
                ref = &(parse->globalReference);
            }
            else {
                ref = &(parse->localReference);
            }
            source.customDataPtr = ref;
            COORD_ChangeCoordinateSystem(
                &source,
                GEODETIC,
                &coordinates);

            if (parse->subtractTerrain == TRUE)
            {
                elevation = coordinates.common.c3;
                TERRAIN_SetToGroundLevel(parse->terrainData,
                                         &coordinates);
                coordinates.common.c3 = elevation - coordinates.common.c3;
            }
        }
        else // CARTESIAN/MGRS
        {
            source.customDataPtr = &(parse->terrainFeatures->m_southWest);
            COORD_ChangeCoordinateSystem(
                &source,
                UNREFERENCED_CARTESIAN,
                &coordinates);
        }
    }
    //If the coordinates are of type "geodetic"
    //read them in and adjust the elevation
    //according to the ground elevation at the point
    else if ((parse->localType == GEODETIC) ||
             ((parse->localType ==
               INVALID_COORDINATE_TYPE) &&
              (parse->globalType == GEODETIC)))
    {
        sscanf(parse->dataBuf, "%lf %lf %lf",
               &(coordinates.latlonalt.longitude),
               &(coordinates.latlonalt.latitude),
               &(coordinates.latlonalt.altitude));

        if (parse->coordinateSystemType == CARTESIAN) {
            ERROR_ReportError("Cannot convert from geodetic coordinates "
                              "to unreferenced cartesian while parsing");
        }
#ifdef MGRS_ADDON
        else if (parse->coordinateSystemType == MGRS)
        {
            ERROR_ReportError("Cannot convert from geodetic coordinates to "
                "MGRS while parsing");
        }
#endif // MGRS_ADDON

        if (parse->subtractTerrain == TRUE)
        {
            elevation = coordinates.common.c3;
            TERRAIN_SetToGroundLevel(parse->terrainData,
                                     &coordinates);
            coordinates.common.c3 = elevation - coordinates.common.c3;
        }
        coordinates.type = GEODETIC;
    }
}

/*! \fn static
  void parseCoordinatesBuilding(TerrainFeaturesParseStruct *parse,
  Coordinates &coordinates)
  \brief Function called to parse coordinates for building faces
  from the string buffer into the relevant structure
  \param parse pointer to the TerrainFeaturesParseStruct structure
  \param coordinates reference to Coordinates structure into which
  the coordinates are to be parsed
*/
// converts coordinates
static
void parseCoordinatesBuilding(TerrainFeaturesParseStruct *parse,
                              Coordinates &coordinates)
{
    Coordinates source;
    Coordinates* ref;
    double elevation;

    //If coordinates are of type "cartesian",
    //read in the coordinates and convert the coordinates
    //to geodetic type using the relevant reference coordinates
    if ((parse->localType == JGIS) ||
            ((parse->localType ==
              INVALID_COORDINATE_TYPE) &&
             (parse->globalType == JGIS)))
    {
        sscanf(parse->dataBuf, "%lf %lf %lf",
               &(source.cartesian.x),
               &(source.cartesian.y),
               &(source.cartesian.z));
        source.type = JGIS;

        if (parse->coordinateSystemType == LATLONALT) {
            if (parse->
                    localReference.latlonalt.latitude == 360) {
                ref = &(parse->
                        globalReference);
            }
            else {
                ref = &(parse->
                        localReference);
            }
            source.customDataPtr = ref;
            Coordinates temp;
            COORD_ChangeCoordinateSystem(
                &source,
                GEODETIC,
                &temp);
            if (parse->subtractTerrain == TRUE)
            {
                elevation = temp.common.c3;
                TERRAIN_SetToGroundLevel(parse->terrainData,
                                         &temp);
                temp.common.c3 = elevation - temp.common.c3;
            }
            memcpy(&coordinates, &temp, sizeof(Coordinates));
        }
        else // CARTESIAN/MGRS
        {
            source.customDataPtr = &(parse->terrainFeatures->m_southWest);
            COORD_ChangeCoordinateSystem(
                &source,
                UNREFERENCED_CARTESIAN,
                &coordinates);
        }
    }
    //If the coordinates are of type "geodetic"
    //read them in and adjust the elevation
    //according to the ground elevation at the point
    else if ((parse->localType == GEODETIC) ||
             ((parse->localType ==
               INVALID_COORDINATE_TYPE) &&
              (parse->globalType == GEODETIC)))
    {
        sscanf(parse->dataBuf, "%lf %lf %lf",
               &(source.latlonalt.longitude),
               &(source.latlonalt.latitude),
               &(source.latlonalt.altitude));

        if (parse->coordinateSystemType == CARTESIAN) {
            ERROR_ReportError("Cannot convert from geodetic coordinates "
                              "to unreferenced cartesian while parsing");
        }
#ifdef MGRS_ADDON
        else if (parse->coordinateSystemType == MGRS)
        {
            ERROR_ReportError("Cannot convert from geodetic coordinates to "
                "MGRS while parsing");
        }
#endif // MGRS_ADDON

        if (parse->subtractTerrain == TRUE)
        {
            elevation = source.common.c3;
            TERRAIN_SetToGroundLevel(parse->terrainData,
                                     &source);
            source.common.c3 = elevation - source.common.c3;
        }
        source.type = GEODETIC;

        memcpy(&coordinates, &source, sizeof(Coordinates));
    }
}

/*! \fn static
  void XMLCALL HandleStartTag(void *data,
  const char *el,
  const char **attr)
  \brief Start tag handler called by expat during second pass
  over the XML files - this is where most of the processing happens
  \param data pointer to the TerrainFeaturesParseStruct structure
  \param el start tag string
  \param attr array of attribute strings for the XML object
  being parsed
*/
static
void XMLCALL HandleStartTag(void *data,
                            const char *el,
                            const char **attr)
{
    TerrainFeaturesParseStruct *parse = (TerrainFeaturesParseStruct *)data;
    int i;
    int hashTablePosition = 0;
    //Character pointer to point to the name of an object
    const char *name = NULL;
    char errorBuf[512];
    //Temp structures used while parsing
    Coordinates *tempCoordinates;
    IntersectionID* tempIntIds;
    RoadSegment* tempRoadSegment;
    FeatureFace* tempFaces;
    Coordinates tempRefs;

    //Check stack depth
    if (parse->stack.depth ==
            TERRAIN_FEATURES_MAX_STACK_DEPTH)
    {
        ERROR_ReportError("Error: Insufficient space on Stack!");
    }

    if (strcmp(el, "Site") == 0) {
        if (parse->stack.depth != 0) {
            ERROR_ReportError("Error: Site tag encountered at non-zero stack "
                              "depth");
        }
        //Add object tag to the tag stack
        parse->stack.
        tagStack[parse->stack.depth] = SITE;
        //Parse site attributes as required
        for (i = 0; attr[i]; i += 2) {
            if (strcmp(attr[i], "ReferencePoint") == 0) {
                memcpy(&tempRefs,
                       &(parse->globalReference),
                       sizeof(Coordinates));

                sscanf(attr[i+1], "%lf %lf %lf",
                       &(parse->
                         globalReference.latlonalt.longitude),
                       &(parse->
                         globalReference.latlonalt.latitude),
                       &(parse->
                         globalReference.latlonalt.altitude));

                if (parse->globalSet == TRUE &&
                        ((tempRefs.latlonalt.longitude !=
                          parse->globalReference.latlonalt.longitude) ||
                         (tempRefs.latlonalt.latitude !=
                          parse->globalReference.latlonalt.latitude)))
                {
                    ERROR_ReportError("Global Reference Coordinates "
                                      "do not match accross files");
                }
                parse->globalSet = TRUE;
            }
            else if (strcmp(attr[i], "CoordinateType") == 0) {
                if (strcmp(attr[i+1], "geodetic") == 0) {
                    parse->globalType = GEODETIC;
                }
                else if (strcmp(attr[i+1], "cartesian") == 0) {
                    parse->globalType = JGIS;
                }
                else {
                    ERROR_ReportError("Unknown coordinateType");
                }
            }
        }
        //printf("Site\n");
    }
    else if (strcmp(el, "Region") == 0) {
        if ((parse->stack.depth < 1) ||
                (parse->stack.
                 tagStack[parse->stack.depth - 1] != SITE))
        {
            ERROR_ReportError("Error: Region tag encountered at incorrect "
                              "position");
        }
        //Add object tag to tag stack
        parse->stack.
        tagStack[parse->stack.depth] = REGION;
        //printf("Region\n");
        //Parse the attributes of the station
        for (i = 0; attr[i]; i += 2) {
            if (strcmp(attr[i], "CoordinateType") == 0) {
                if (strcmp(attr[i+1], "geodetic") == 0) {
                    parse->localType = GEODETIC;
                }
                else if (strcmp(attr[i+1], "cartesian") == 0) {
                    parse->localType = JGIS;
                }
                else {
                    ERROR_ReportError("Unknown coordinateType");
                }
            }
        }
    }
    else if (strcmp(el, "position") == 0) {
        if ((parse->stack.depth < 2) ||
                ((parse->stack.
                  tagStack[parse->stack.depth - 1] != REGION) &&
                 (parse->stack.
                  tagStack[parse->stack.depth - 1] != PARK) &&
                 (parse->stack.
                  tagStack[parse->stack.depth - 1] != STATION) &&
                 (parse->stack.
                  tagStack[parse->stack.depth - 1] != FACE) &&
                 (parse->stack.
                  tagStack[parse->stack.depth - 1] != CLOUD)))
        {
            ERROR_ReportError("Error: position tag encountered at incorrect "
                              "position");
        }
        //Add object tag to tag stack
        parse->stack.
        tagStack[parse->stack.depth] = POSITION;
        //printf("position\n");

        //Parse coordinates to tempCoordinates structure
        if (parse->stack.
                tagStack[parse->stack.depth - 1]
                == REGION)
        {
            if ((parse->prevTag != INVALID_TAG) &&
                    (parse->prevTag != POSITION))
            {
                ERROR_ReportError("Error: malformed park or station "
                                  "object");
            }
            parse->stack.
            objectStack[parse->stack.depth] = NULL;
        }

        //Add the coordinates to the temporary coordinates array
        else if ((parse->stack.
                  tagStack[parse->stack.depth - 1]
                  == PARK)  ||
                 (parse->stack.
                  tagStack[parse->stack.depth - 1]
                  == STATION) ||
                 (parse->stack.
                  tagStack[parse->stack.depth - 1]
                  == FACE))
        {
            if ((parse->prevTag != INVALID_TAG) &&
                    (parse->prevTag != POSITION))
            {
                ERROR_ReportError("Error: malformed park or station "
                                  "object");
            }
            //Resize the temporary coordinates array if required
            if (parse->numCoordinates ==
                    parse->maxCoordinates)
            {
                parse->maxCoordinates =
                    parse->maxCoordinates*2;
                tempCoordinates = parse->coordinateList;
                parse->coordinateList =
                    (Coordinates *)MEM_malloc(sizeof(Coordinates) *
                                              parse->
                                              maxCoordinates);
                memcpy(parse->coordinateList,
                       tempCoordinates,
                       sizeof(Coordinates) *
                       parse->numCoordinates);
                MEM_free(tempCoordinates);
            }
            //For parks and stations, resize the temporary
            //intersection ID array if required
            if ((parse->stack.
                    tagStack[parse->stack.depth - 1]
                    == PARK)  ||
                    (parse->stack.
                     tagStack[parse->stack.depth - 1]
                     == STATION))
            {
                if (parse->maxIntIds <
                        parse->maxCoordinates)
                {
                    parse->maxIntIds =
                        parse->maxCoordinates;
                    tempIntIds = parse->intIDs;
                    parse->intIDs = (IntersectionID *)
                                    MEM_malloc(sizeof(IntersectionID) *
                                               parse->
                                               maxIntIds);
                    if (parse->numCoordinates > 0) {
                        memcpy(parse->intIDs,
                               tempIntIds,
                               sizeof(IntersectionID) *
                               parse->numCoordinates);
                        MEM_free(tempIntIds);
                    }
                }
                //Read in the exit intersection id if it exists
                if (attr[0] && (strcmp(attr[0], "ExitIntersectionId") == 0)) {
                    hashTablePosition = Find(parse, attr[1]);
                    if (parse->
                            hashTable->cells[hashTablePosition].entryState ==
                            EMPTY)
                    {
                        sprintf(errorBuf,
                                "Undefined intersection ID found: %s",
                                attr[1]);
                        ERROR_ReportError(errorBuf);

                    }
                    else {
                        //read in the id
                        parse->intIDs
                        [parse->numCoordinates] =
                            parse->
                            hashTable->cells[hashTablePosition].objectID;
                    }
                }
                else {
                    parse->intIDs
                    [parse->numCoordinates] =
                        INVALID_INTERSECTION;
                }
            }
            parse->stack.
            objectStack[parse->stack.depth] = NULL;
            parse->numCoordinates++;
        }
        //Reset the data buffer that will be used to read in the coordinates
        parse->dataBuf[0] = 0;
    }
    else if (strcmp(el, "Park") == 0) {
        if ((parse->stack.depth < 1) ||
                (parse->stack.
                 tagStack[parse->stack.depth - 1] != SITE))
        {
            ERROR_ReportError("Error: Park tag encountered at incorrect "
                              "position");
        }
        //Add the object tag to the tag stack
        parse->stack.
        tagStack[parse->stack.depth] = PARK;
        //printf("Park\n");

        //Parse the attributes of the park
        for (i = 0; attr[i]; i += 2) {
            if (strcmp(attr[i], "id") == 0) {
                hashTablePosition = Find(parse, attr[i+1]);
                if ((parse->hashTable->
                        cells[hashTablePosition].objectState == COMPLETE) ||
                        (parse->hashTable->
                         cells[hashTablePosition].objectType
                         != PARK))
                {
                    sprintf(errorBuf, "Id repetition: %s", attr[i+1]);
                    ERROR_ReportError(errorBuf);
                }
                parse->terrainFeatures->m_parks
                [parse->hashTable->
                 cells[hashTablePosition].objectID].XML_ID =
                     (char *)MEM_malloc(sizeof(char) *
                                        (strlen(attr[i+1]) + 1));
                strcpy(parse->terrainFeatures->m_parks
                       [parse->hashTable->
                        cells[hashTablePosition].objectID].XML_ID,
                       attr[i+1]);
            }
            else if (strcmp(attr[i], "ReferencePoint") == 0) {
                sscanf(attr[i+1], "%lf %lf %lf",
                       &(parse->
                         localReference.latlonalt.longitude),
                       &(parse->
                         localReference.latlonalt.latitude),
                       &(parse->
                         localReference.latlonalt.altitude));
            }
            else if (strcmp(attr[i], "CoordinateType") == 0) {
                if (strcmp(attr[i+1], "geodetic") == 0) {
                    parse->localType = GEODETIC;
                }
                else if (strcmp(attr[i+1], "cartesian") == 0) {
                    parse->localType = JGIS;
                }
                else {
                    ERROR_ReportError("Unknown coordinateType");
                }
            }
        }

        //Setup the new park structure that will be used to
        //hold the information of this park
        parse->terrainFeatures->m_parks
        [parse->hashTable->
         cells[hashTablePosition].objectID].ID =
             parse->hashTable->
             cells[hashTablePosition].objectID;
        //Add the park structure to the object stack
        parse->stack.
        objectStack[parse->stack.depth] =
            &(parse->terrainFeatures->m_parks
              [parse->hashTable->
               cells[hashTablePosition].objectID]);
        parse->hashTable->
        cells[hashTablePosition].objectState = COMPLETE;

        parse->terrainFeatures->m_numParks++;
    }
    else if (strcmp(el, "representative") == 0) {
        if ((parse->stack.depth < 2) ||
                ((parse->stack.
                  tagStack[parse->stack.depth - 1] != PARK) &&
                 (parse->stack.
                  tagStack[parse->stack.depth - 1] != STATION)))
        {
            ERROR_ReportError("Error: representative tag encountered at "
                              "incorrect position");
        }
        //Add the object tag to the tag stack
        parse->stack.
        tagStack[parse->stack.depth] = REPRESENTATIVE;
        //printf("representative\n");
        if ((parse->stack.
                tagStack[parse->stack.depth - 1]
                == PARK) ||
                (parse->stack.
                 tagStack[parse->stack.depth - 1]
                 == STATION))
        {
            if (parse->prevTag != POSITION)
            {
                ERROR_ReportError("Error: malformed park or station object");
            }
            //Reset the data buffer
            parse->dataBuf[0] = 0;
            parse->stack.
            objectStack[parse->stack.depth] = NULL;
        }
    }
    else if (strcmp(el, "Station") == 0) {
        if ((parse->stack.depth < 1) ||
                (parse->stack.
                 tagStack[parse->stack.depth - 1] != SITE))
        {
            ERROR_ReportError("Error: Station tag encountered at incorrect "
                              "position");
        }
        //Add the object tag to the tag stack
        parse->stack.
        tagStack[parse->stack.depth] = STATION;
        //printf("Station\n");

        //Parse the attributes of the station
        for (i = 0; attr[i]; i += 2) {
            if (strcmp(attr[i], "id") == 0) {
                hashTablePosition = Find(parse, attr[i+1]);
                if ((parse->hashTable->
                        cells[hashTablePosition].objectState == COMPLETE) ||
                        (parse->hashTable->
                         cells[hashTablePosition].objectType
                         != STATION))
                {
                    sprintf(errorBuf, "Id repetition: %s", attr[i+1]);
                    ERROR_ReportError(errorBuf);
                }
                parse->terrainFeatures->m_stations
                [parse->hashTable->
                 cells[hashTablePosition].objectID].XML_ID =
                     (char *)MEM_malloc(sizeof(char) *
                                        (strlen(attr[i+1]) + 1));
                strcpy(parse->terrainFeatures->m_stations
                       [parse->hashTable->
                        cells[hashTablePosition].objectID].XML_ID,
                       attr[i+1]);
            }
            else if (strcmp(attr[i], "ReferencePoint") == 0) {
                sscanf(attr[i+1], "%lf %lf %lf",
                       &(parse->
                         localReference.latlonalt.longitude),
                       &(parse->
                         localReference.latlonalt.latitude),
                       &(parse->
                         localReference.latlonalt.altitude));
            }
            else if (strcmp(attr[i], "CoordinateType") == 0) {
                if (strcmp(attr[i+1], "geodetic") == 0) {
                    parse->localType = GEODETIC;
                }
                else if (strcmp(attr[i+1], "cartesian") == 0) {
                    parse->localType = JGIS;
                }
                else {
                    ERROR_ReportError("Unknown coordinateType");
                }
            }
        }

        //Setup the new park structure that will be used to
        //hold the information of this park
        parse->terrainFeatures->m_stations
        [parse->hashTable->
         cells[hashTablePosition].objectID].ID =
             parse->hashTable->
             cells[hashTablePosition].objectID;
        //Add the station struct to the object stack
        parse->stack.
        objectStack[parse->stack.depth] =
            &(parse->terrainFeatures->m_stations
              [parse->hashTable->
               cells[hashTablePosition].objectID]);
        parse->hashTable->
        cells[hashTablePosition].objectState = COMPLETE;

        parse->terrainFeatures->m_numStations++;
    }
    else if (strcmp(el, "Building") == 0) {
        if ((parse->stack.depth < 1) ||
                (parse->stack.tagStack[parse->stack.depth - 1] != SITE))
        {
            ERROR_ReportError("Error: Building tag encountered at incorrect "
                              "position");
        }
        //Add the object tag to the object stack
        parse->stack.tagStack[parse->stack.depth] = BUILDING;
        //printf("Building\n");

        //Parse the building attributes
        for (i = 0; attr[i]; i += 2) {
            if (strcmp(attr[i], "id") == 0) {
                hashTablePosition = Find(parse, attr[i+1]);
                if ((parse->hashTable->
                        cells[hashTablePosition].objectState == COMPLETE) ||
                        (parse->hashTable->
                         cells[hashTablePosition].objectType != BUILDING))
                {
                    sprintf(errorBuf, "Id repetition: %s", attr[i+1]);
                    ERROR_ReportError(errorBuf);
                }
                parse->terrainFeatures->m_buildings
                [parse->hashTable->
                 cells[hashTablePosition].objectID].XML_ID =
                     (char *) MEM_malloc(sizeof(char) *
                                         (strlen(attr[i+1]) + 1));
                strcpy(parse->terrainFeatures->m_buildings
                       [parse->hashTable->
                        cells[hashTablePosition].objectID].XML_ID,
                       attr[i+1]);
            }
            else if (strcmp(attr[i], "Name") == 0) {
                name = attr[i+1];
            }
            else if (strcmp(attr[i], "ReferencePoint") == 0) {
                sscanf(attr[i+1], "%lf %lf %lf",
                       &(parse->
                         buildingReference.latlonalt.longitude),
                       &(parse->
                         buildingReference.latlonalt.latitude),
                       &(parse->
                         buildingReference.latlonalt.altitude));
            }
            else if (strcmp(attr[i], "CoordinateType") == 0) {
                if (strcmp(attr[i+1], "geodetic") == 0) {
                    parse->buildingType = GEODETIC;
                }
                else if (strcmp(attr[i+1], "cartesian") == 0) {
                    parse->buildingType = JGIS;
                }
                else {
                    ERROR_ReportError("Unknown coordinateType");
                }
            }
        }
        parse->terrainFeatures->m_buildings
        [parse->hashTable->
         cells[hashTablePosition].objectID].featureName = NULL;

        if (name) {
            parse->terrainFeatures->m_buildings
            [parse->hashTable->
             cells[hashTablePosition].objectID].featureName =
                 (char *)MEM_malloc(sizeof(char) * (strlen(name) + 1));
            strcpy(parse->terrainFeatures->m_buildings
                   [parse->hashTable->
                    cells[hashTablePosition].objectID].featureName,
                   name);
            name = NULL;
        }

        //Add the building structure to the object stack
        parse->stack.
        objectStack[parse->stack.depth] =
            &(parse->terrainFeatures->m_buildings
              [parse->hashTable->
               cells[hashTablePosition].objectID]);
        parse->hashTable->
        cells[hashTablePosition].objectState = COMPLETE;

        parse->terrainFeatures->m_numBuildings++;
    }
    else if (strcmp(el, "Foliage") == 0) {
        // TODO:  Merge code with Building processing and use if-else
        if ((parse->stack.depth < 1) ||
                (parse->stack.tagStack[parse->stack.depth - 1] != SITE))
        {
            ERROR_ReportError("Error: Foliage tag encountered at incorrect "
                              "position");
        }
        //Add the object tag to the object stack
        parse->stack.tagStack[parse->stack.depth] = FOLIAGE;
        //printf("Foliage\n");

        //Parse the foliage attributes
        for (i = 0; attr[i]; i += 2) {
            if (strcmp(attr[i], "id") == 0) {
                hashTablePosition = Find(parse, attr[i+1]);
                if ((parse->hashTable->
                        cells[hashTablePosition].objectState == COMPLETE) ||
                        (parse->hashTable->
                         cells[hashTablePosition].objectType != FOLIAGE))
                {
                    sprintf(errorBuf, "Id repetition: %s", attr[i+1]);
                    ERROR_ReportError(errorBuf);
                }
                parse->terrainFeatures->m_foliage
                [parse->hashTable->
                 cells[hashTablePosition].objectID].XML_ID =
                     (char *) MEM_malloc(sizeof(char) *
                                         (strlen(attr[i+1]) + 1));
                strcpy(parse->terrainFeatures->m_foliage
                       [parse->hashTable->
                        cells[hashTablePosition].objectID].XML_ID,
                       attr[i+1]);
            }
            else if (strcmp(attr[i], "Name") == 0) {
                name = attr[i+1];
            }
            else if (strcmp(attr[i], "ReferencePoint") == 0) {
                sscanf(attr[i+1], "%lf %lf %lf",
                       &(parse->
                         foliageReference.latlonalt.longitude),
                       &(parse->
                         foliageReference.latlonalt.latitude),
                       &(parse->
                         foliageReference.latlonalt.altitude));
            }
            else if (strcmp(attr[i], "CoordinateType") == 0) {
                if (strcmp(attr[i+1], "geodetic") == 0) {
                    parse->foliageType = GEODETIC;
                }
                else if (strcmp(attr[i+1], "cartesian") == 0) {
                    parse->foliageType = JGIS;
                }
                else {
                    ERROR_ReportError("Unknown coordinateType");
                }
            }
            else if (strcmp(attr[i], "density") == 0) {
                double density;
                sscanf(attr[i + 1], "%lf", &density);
                if (density < 0.0 || density > 1.0) {
                    ERROR_ReportError("Invalid density");
                }
                parse->terrainFeatures->m_foliage
                [parse->hashTable->
                 cells[hashTablePosition].objectID].density = density;
            }
        }
        parse->terrainFeatures->m_foliage
        [parse->hashTable->
         cells[hashTablePosition].objectID].featureName = NULL;

        if (name) {
            parse->terrainFeatures->m_foliage
            [parse->hashTable->
             cells[hashTablePosition].objectID].featureName =
                 (char *)MEM_malloc(sizeof(char) * (strlen(name) + 1));
            strcpy(parse->terrainFeatures->m_foliage
                   [parse->hashTable->
                    cells[hashTablePosition].objectID].featureName,
                   name);
            name = NULL;
        }

        //Add the foliage structure to the object stack
        parse->stack.
        objectStack[parse->stack.depth] =
            &(parse->terrainFeatures->m_foliage
              [parse->hashTable->
               cells[hashTablePosition].objectID]);
        parse->hashTable->
        cells[hashTablePosition].objectState = COMPLETE;

        parse->terrainFeatures->m_numFoliage++;
    }
    else if (strcmp(el, "face") == 0) {
        if ((parse->stack.depth < 2) ||
                (parse->stack.tagStack[parse->stack.depth - 1] != BUILDING
                 && parse->stack.tagStack[parse->stack.depth - 1] != FOLIAGE))
        {
            ERROR_ReportError("Error: face tag encountered at incorrect "
                              "position");
        }
        //Add the object tag to the tag stack
        parse->stack.tagStack[parse->stack.depth] = FACE;
        //printf("face\n");

        //Resize the temporary building face array
        //used to store building faces, if necessary
        if (parse->numFaces == parse->maxFaces)
        {
            parse->maxFaces = parse->maxFaces*2;
            tempFaces = parse->faces;
            parse->faces =
                (FeatureFace *)MEM_malloc(sizeof(FeatureFace) *
                                          parse->
                                          maxFaces);
            memcpy(parse->faces,
                   tempFaces,
                   sizeof(FeatureFace) *
                   parse->numFaces);
            MEM_free(tempFaces);
        }
        // TODO:  Determine whether we can just do a direct
        // coord1 = coord2 assignment in next two blocks.
        if (parse->stack.tagStack[parse->stack.depth - 1] == BUILDING)
        {
            //Parse in the building face attributes
            parse->localReference.latlonalt.longitude =
                parse->buildingReference.latlonalt.longitude;
            parse->localReference.latlonalt.latitude =
                parse->buildingReference.latlonalt.latitude;
            parse->localReference.latlonalt.altitude =
                parse->buildingReference.latlonalt.altitude;
            parse->localType = parse->buildingType;
        }
        else
            if (parse->stack.tagStack[parse->stack.depth - 1] == FOLIAGE)
            {
                //Parse in the foliage face attributes
                parse->localReference.latlonalt.longitude =
                    parse->foliageReference.latlonalt.longitude;
                parse->localReference.latlonalt.latitude =
                    parse->foliageReference.latlonalt.latitude;
                parse->localReference.latlonalt.altitude =
                    parse->foliageReference.latlonalt.altitude;
                parse->localType = parse->foliageType;
            }
            else { assert(0); }
        //Fill in attributes
        for (i = 0; attr[i]; i += 2) {
            if (strcmp(attr[i], "id") == 0) {
                parse->faces
                [parse->numFaces].XML_ID =
                    (char *)MEM_malloc(sizeof(char) *
                                       (strlen(attr[i+1]) + 1));
                strcpy(parse->faces
                       [parse->numFaces].XML_ID,
                       attr[i+1]);
            }
            else if (strcmp(attr[i], "ReferencePoint") == 0) {
                sscanf(attr[i+1], "%lf %lf %lf",
                       &(parse->
                         localReference.latlonalt.longitude),
                       &(parse->
                         localReference.latlonalt.latitude),
                       &(parse->
                         localReference.latlonalt.altitude));
            }
            else if (strcmp(attr[i], "CoordinateType") == 0) {
                if (strcmp(attr[i+1], "geodetic") == 0) {
                    parse->localType = GEODETIC;
                }
                else if (strcmp(attr[i+1], "cartesian") == 0) {
                    parse->localType = JGIS;
                }
                else {
                    ERROR_ReportError("Unknown coordinateType");
                }
            }
        }
        parse->faces[parse->numFaces].thickness = 0;
        //Add the building face to the object stack
        parse->stack.
        objectStack[parse->stack.depth] =
            &(parse->
              faces[parse->numFaces]);
        parse->numFaces++;
    }
    else if (strcmp(el, "thickness") == 0) {
        if ((parse->stack.depth < 2) ||
                ((parse->stack.
                  tagStack[parse->stack.depth - 1] != FACE) &&
                 (parse->stack.
                  tagStack[parse->stack.depth - 1] != CLOUD)))
        {
            ERROR_ReportError("Error: thickness tag encountered at incorrect "
                              "position");
        }
        //Add the object tag to the tag stack
        parse->stack.
        tagStack[parse->stack.depth] = THICKNESS;
        //printf("thickness\n");
        if (parse->stack.
                tagStack[parse->stack.depth - 1]
                == FACE)
        {
            if (parse->prevTag != POSITION)
            {
                ERROR_ReportError("Error: malformed building face object");
            }
            parse->dataBuf[0] = 0;
            parse->stack.
            objectStack[parse->stack.depth] = NULL;
        }

    }
    else if (strcmp(el, "Cloud") == 0) {
        if ((parse->stack.depth < 1) ||
                (parse->stack.
                 tagStack[parse->stack.depth - 1] != SITE))
        {
            ERROR_ReportError("Error: Cloud tag encountered at incorrect "
                              "position");
        }
        //Add the object tag to the tag stack
        parse->stack.
        tagStack[parse->stack.depth] = CLOUD;
        //printf("Cloud\n");
    }
    else if (strcmp(el, "Street_Segment") == 0) {
        if ((parse->stack.depth < 1) ||
                (parse->stack.
                 tagStack[parse->stack.depth - 1] != SITE))
        {
            ERROR_ReportError("Error: Street_Segment tag encountered at "
                              "incorrect position");
        }
        //Add the object tag to the tag stack
        parse->stack.
        tagStack[parse->stack.depth] = STREET_SEGMENT;
        //printf("Street_Segment\n");

        //Parse the street segment's attributes
        for (i = 0; attr[i]; i += 2) {
            if (strcmp(attr[i], "id") == 0) {
                hashTablePosition = Find(parse, attr[i+1]);
                if ((parse->hashTable->
                        cells[hashTablePosition].objectState == COMPLETE) ||
                        (parse->hashTable->
                         cells[hashTablePosition].objectType
                         != STREET_SEGMENT))
                {
                    sprintf(errorBuf, "Id repetition: %s", attr[i+1]);
                    ERROR_ReportError(errorBuf);

                }
                parse->terrainFeatures->m_roadSegments
                [parse->hashTable->
                 cells[hashTablePosition].objectID].XML_ID =
                     (char *)MEM_malloc(sizeof(char) *
                                        (strlen(attr[i+1]) + 1));
                strcpy(parse->terrainFeatures->m_roadSegments
                       [parse->hashTable->
                        cells[hashTablePosition].objectID].XML_ID,
                       attr[i+1]);
            }
            else if (strcmp(attr[i], "Name") == 0) {
                name = attr[i+1];
            }
            else if (strcmp(attr[i], "ReferencePoint") == 0) {
                sscanf(attr[i+1], "%lf %lf %lf",
                       &(parse->
                         localReference.latlonalt.longitude),
                       &(parse->
                         localReference.latlonalt.latitude),
                       &(parse->
                         localReference.latlonalt.altitude));
            }
            else if (strcmp(attr[i], "CoordinateType") == 0) {
                if (strcmp(attr[i+1], "geodetic") == 0) {
                    parse->localType = GEODETIC;
                }
                else if (strcmp(attr[i+1], "cartesian") == 0) {
                    parse->localType = JGIS;
                }
                else {
                    ERROR_ReportError("Unknown coordinateType");
                }
            }
        }

        parse->terrainFeatures->m_roadSegments
        [parse->hashTable->
         cells[hashTablePosition].objectID].streetName = NULL;
        if (name) {
            parse->terrainFeatures->m_roadSegments
            [parse->hashTable->
             cells[hashTablePosition].objectID].streetName =
                 (char *)MEM_malloc(sizeof(char) * (strlen(name) + 1));
            strcpy(parse->terrainFeatures->m_roadSegments
                   [parse->hashTable->
                    cells[hashTablePosition].objectID].streetName,
                   name);
            name = NULL;
        }
        parse->terrainFeatures->m_roadSegments
        [parse->hashTable->
         cells[hashTablePosition].objectID].ID =
             parse->hashTable->
             cells[hashTablePosition].objectID;
        //Setup the list of related street segments to be
        //related if they are presented as being related
        //Translation: node in the middle of a street segment
        parse->terrainFeatures->m_roadSegments
        [parse->hashTable->
         cells[hashTablePosition].objectID].roadSegmentVar = NULL;
        //Add the street segment structure to the object stack
        parse->stack.
        objectStack[parse->stack.depth] =
            &(parse->terrainFeatures->m_roadSegments
              [parse->hashTable->
               cells[hashTablePosition].objectID]);
        parse->hashTable->
        cells[hashTablePosition].objectState = COMPLETE;

        parse->terrainFeatures->m_numRoadSegments++;
    }
    else if (strcmp(el, "firstNode") == 0) {
        if ((parse->stack.depth < 2) ||
                ((parse->stack.
                  tagStack[parse->stack.depth - 1]
                  != STREET_SEGMENT) &&
                 (parse->stack.
                  tagStack[parse->stack.depth - 1]
                  != RAILROAD)))
        {
            ERROR_ReportError("Error: firstNode tag encountered at "
                              "incorrect position");
        }
        //Add the object tag to the tag stack
        parse->stack.
        tagStack[parse->stack.depth] = FIRST_NODE;
        //printf("firstNode\n");
        if (parse->stack.
                tagStack[parse->stack.depth - 1]
                == STREET_SEGMENT) {
            if (parse->prevTag != INVALID_TAG) {
                ERROR_ReportError("Error: malformed street_segment "
                                  "object");
            }
            //Parse the intersection Id of the first node
            if (attr[0] && (strcmp(attr[0], "objectRef") == 0)) {
                hashTablePosition = Find(parse, attr[1]);
                if (parse->
                        hashTable->cells[hashTablePosition].entryState == EMPTY)
                {
                    sprintf(errorBuf,
                            "Undefined intersection ID found: %sa",
                            attr[1]);
                    ERROR_ReportError(errorBuf);
                }
                else {
                    ((RoadSegment *)parse->stack.
                     objectStack[parse->stack.depth -1])->
                    firstIntersection =
                        parse->hashTable->
                        cells[hashTablePosition].objectID;
                }
            }
            else {
                ERROR_ReportError("firstNode tag missing objectRef "
                                  "attribute");
            }
            if (parse->numCoordinates != 0) {
                ERROR_ReportError("Malformed XML file");
            }
            //Reset the data buffer
            parse->dataBuf[0] = 0;
            parse->stack.
            objectStack[parse->stack.depth] =
                &(parse->
                  coordinateList[parse->numCoordinates]);
            parse->numCoordinates++;
        }
    }
    else if (strcmp(el, "node") == 0) {
        if ((parse->stack.depth < 2) ||
                ((parse->stack.
                  tagStack[parse->stack.depth - 1]
                  != STREET_SEGMENT) &&
                 (parse->stack.
                  tagStack[parse->stack.depth - 1]
                  != RAILROAD)))
        {
            ERROR_ReportError("Error: node tag encountered at "
                              "incorrect position");
        }
        //Add the object tag to the tag stack
        parse->stack.
        tagStack[parse->stack.depth] = NODE;
        //printf("node\n");
        if (parse->stack.
                tagStack[parse->stack.depth - 1]
                == STREET_SEGMENT) {
            if ((parse->prevTag != FIRST_NODE) &&
                    (parse->prevTag != NODE))
            {
                sprintf(errorBuf,
                        "Error: malformed street_segment object %d",
                        parse->prevTag);
                ERROR_ReportError(errorBuf);

            }
            //If there is a node in the middle, split
            //the street segments, replicate the relevant attributes
            //and set up the street segments are related segments
            if (attr[0] && (strcmp(attr[0], "objectRef") == 0)) {
                hashTablePosition = Find(parse, attr[1]);
                if (parse->
                        hashTable->cells[hashTablePosition].entryState == EMPTY)
                {
                    sprintf(errorBuf,
                            "Undefined intersection ID found: %s",
                            attr[1]);
                    ERROR_ReportError(errorBuf);

                }
                else {
                    // should check that referenced object type is intersection

                    //Split the street segments and replicate the
                    //relevant attributes
                    tempRoadSegment =
                        ((RoadSegment *)parse->stack.
                         objectStack[parse->stack.depth-1]);
                    parse->terrainFeatures->m_roadSegments
                    [parse->maxStreetSegObjects +
                     parse->numExtraStreetSegs].XML_ID
                    = NULL;
                    parse->terrainFeatures->m_roadSegments
                    [parse->maxStreetSegObjects +
                     parse->numExtraStreetSegs].streetName
                    = NULL;
                    if (tempRoadSegment->streetName) {
                        parse->terrainFeatures->m_roadSegments
                        [parse->maxStreetSegObjects +
                         parse->numExtraStreetSegs].streetName =
                             (char *)MEM_malloc(sizeof(char) *
                                                (strlen(tempRoadSegment->
                                                        streetName)
                                                 + 1));
                        strcpy(parse->terrainFeatures->m_roadSegments
                               [parse->maxStreetSegObjects +
                                parse->numExtraStreetSegs].
                               streetName,
                               tempRoadSegment->streetName);
                    }
                    parse->terrainFeatures->m_roadSegments
                    [parse->maxStreetSegObjects +
                     parse->numExtraStreetSegs].ID =
                         parse->maxStreetSegObjects +
                         parse->numExtraStreetSegs;
                    parse->terrainFeatures->m_roadSegments
                    [parse->maxStreetSegObjects +
                     parse->numExtraStreetSegs].firstIntersection =
                         parse->hashTable->
                         cells[hashTablePosition].objectID;
                    parse->terrainFeatures->m_roadSegments
                    [parse->maxStreetSegObjects +
                     parse->numExtraStreetSegs].roadSegmentVar =
                         (TerrainFeaturesRoadLinks *)
                         MEM_malloc(sizeof(TerrainFeaturesRoadLinks));
                    ((TerrainFeaturesRoadLinks *)parse->terrainFeatures->
                     m_roadSegments[parse->maxStreetSegObjects +
                                    parse->numExtraStreetSegs].roadSegmentVar)->
                    prevRoad = tempRoadSegment;
                    ((TerrainFeaturesRoadLinks *)parse->terrainFeatures->
                     m_roadSegments[parse->maxStreetSegObjects +
                                    parse->numExtraStreetSegs].roadSegmentVar)->
                    nextRoad = NULL;
                    ((TerrainFeaturesRoadLinks *)parse->terrainFeatures->
                     m_roadSegments[parse->maxStreetSegObjects +
                                    parse->numExtraStreetSegs].roadSegmentVar)->
                    isSecondLast = FALSE;
                    if (tempRoadSegment->roadSegmentVar == NULL) {
                        tempRoadSegment->roadSegmentVar =
                            MEM_malloc(sizeof(TerrainFeaturesRoadLinks));
                        ((TerrainFeaturesRoadLinks *)tempRoadSegment->
                         roadSegmentVar)->prevRoad = NULL;
                    }
                    ((TerrainFeaturesRoadLinks *)tempRoadSegment->
                     roadSegmentVar)->nextRoad =
                         &(parse->terrainFeatures->m_roadSegments
                           [parse->maxStreetSegObjects +
                            parse->numExtraStreetSegs]);
                    tempRoadSegment->secondIntersection =
                        parse->hashTable->
                        cells[hashTablePosition].objectID;
                    ((TerrainFeaturesRoadLinks *)tempRoadSegment->
                     roadSegmentVar)->isSecondLast = TRUE;

                    //Replace the previous street segment on the
                    //stack with this one
                    parse->stack.
                    objectStack[parse->stack.depth-1] =
                        &(parse->terrainFeatures->m_roadSegments
                          [parse->maxStreetSegObjects +
                           parse->numExtraStreetSegs]);

                    parse->numExtraStreetSegs++;
                    parse->terrainFeatures->m_numRoadSegments++;
                }
            }
            if (parse->numCoordinates == 0) {
                ERROR_ReportError("Malformed XML file");
            }
            //Resize the temporary coordinates array if required
            if (parse->numCoordinates ==
                    parse->maxCoordinates)
            {
                parse->maxCoordinates =
                    parse->maxCoordinates*2;
                tempCoordinates = parse->coordinateList;
                parse->coordinateList =
                    (Coordinates *)MEM_malloc(sizeof(Coordinates) *
                                              parse->
                                              maxCoordinates);
                memcpy(parse->coordinateList,
                       tempCoordinates,
                       sizeof(Coordinates) *
                       parse->numCoordinates);
                MEM_free(tempCoordinates);
            }
            //Reset the data buffer to read in the coordinates
            parse->dataBuf[0] = 0;
            parse->stack.
            objectStack[parse->stack.depth] =
                &(parse->
                  coordinateList[parse->numCoordinates]);
            parse->numCoordinates++;
        }
    }
    else if (strcmp(el, "lastNode") == 0) {
        if ((parse->stack.depth < 2) ||
                ((parse->stack.
                  tagStack[parse->stack.depth - 1]
                  != STREET_SEGMENT) &&
                 (parse->stack.
                  tagStack[parse->stack.depth - 1]
                  != RAILROAD)))
        {
            ERROR_ReportError("Error: lastNode tag encountered at incorrect "
                              "position");
        }
        //Add the object tag to the tag stack
        parse->stack.
        tagStack[parse->stack.depth] = LAST_NODE;
        //printf("lastNode\n");
        if (parse->stack.
                tagStack[parse->stack.depth - 1]
                == STREET_SEGMENT) {
            if ((parse->prevTag != FIRST_NODE) &&
                    (parse->prevTag != NODE))
            {
                ERROR_ReportError("Error: malformed street_segment object");
            }

            //Parse the intersection ID of the last node
            if (attr[0] && (strcmp(attr[0], "objectRef") == 0)) {
                hashTablePosition = Find(parse, attr[1]);
                if (parse->
                        hashTable->cells[hashTablePosition].entryState == EMPTY)
                {
                    sprintf(errorBuf,
                            "Undefined intersection ID found: %s",
                            attr[1]);
                    ERROR_ReportError(errorBuf);

                }
                else {
                    ((RoadSegment *)parse->stack.
                     objectStack[parse->
                                 stack.depth - 1])->secondIntersection =
                                     parse->hashTable->
                                     cells[hashTablePosition].objectID;
                }
            }
            else {
                ERROR_ReportError("secondNode tag missing objectRef "
                                  "attribute");
            }

            if (parse->numCoordinates == 0) {
                ERROR_ReportError("Malformed XML file");
            }
            //Resize the temporary coordinates array if required
            if (parse->numCoordinates ==
                    parse->maxCoordinates)
            {
                parse->maxCoordinates =
                    parse->maxCoordinates*2;
                tempCoordinates = parse->coordinateList;
                parse->coordinateList =
                    (Coordinates *)MEM_malloc(sizeof(Coordinates) *
                                              parse->
                                              maxCoordinates);
                memcpy(parse->coordinateList,
                       tempCoordinates,
                       sizeof(Coordinates) *
                       parse->numCoordinates);
                MEM_free(tempCoordinates);
            }
            //Reset the data buffer
            parse->dataBuf[0] = 0;
            parse->stack.objectStack[parse->stack.depth] =
                &(parse->coordinateList
                  [parse->numCoordinates]);
            parse->numCoordinates++;
        }
    }
    else if (strcmp(el, "width") == 0) {
        if ((parse->stack.depth < 2) ||
                ((parse->stack.
                  tagStack[parse->stack.depth - 1]
                  != STREET_SEGMENT) &&
                 (parse->stack.
                  tagStack[parse->stack.depth - 1]
                  != RAILROAD)))
        {
            ERROR_ReportError("Error: width tag encountered at incorrect "
                              "position");
        }
        //Add the object tag to the tag stack
        parse->stack.
        tagStack[parse->stack.depth] = WIDTH;
        //printf("width\n");
        if (parse->stack.
                tagStack[parse->stack.depth - 1]
                == STREET_SEGMENT) {
            if (parse->prevTag != LAST_NODE)
            {
                ERROR_ReportError("Error: malformed street_segment object");
            }
            //Reset the data buffer
            parse->dataBuf[0] = 0;
            parse->stack.
            objectStack[parse->stack.depth] = NULL;
        }
    }
    else if (strcmp(el, "Railroad") == 0) {
        if ((parse->stack.depth < 1) ||
                (parse->stack.
                 tagStack[parse->stack.depth - 1] != SITE))
        {
            ERROR_ReportError("Error: Railroad tag encountered at incorrect "
                              "position");
        }
        //Add the object tag to the tag stack
        parse->stack.
        tagStack[parse->stack.depth] = RAILROAD;
        //printf("Railroad\n");
    }
    else if (strcmp(el, "Intersection") == 0) {
        if ((parse->stack.depth < 1) ||
                (parse->stack.
                 tagStack[parse->stack.depth - 1] != SITE))
        {
            ERROR_ReportError("Error: Interserction tag encountered at "
                              "incorrect position");
        }
        //Add the object tag to the tag stack
        parse->stack.
        tagStack[parse->stack.depth] = INTERSECTION;
        //printf("Intersection\n");

        //Parse the intersection attributes
        for (i = 0; attr[i]; i += 2) {
            if (strcmp(attr[i], "id") == 0) {
                hashTablePosition = Find(parse, attr[i+1]);
                if ((parse->hashTable->
                        cells[hashTablePosition].objectState == COMPLETE) ||
                        (parse->hashTable->
                         cells[hashTablePosition].objectType != INTERSECTION))
                {
                    sprintf(errorBuf, "Id repetition: %s", attr[i+1]);
                    ERROR_ReportError(errorBuf);

                }
                parse->terrainFeatures->m_intersections
                [parse->hashTable->
                 cells[hashTablePosition].objectID].XML_ID =
                     (char *)MEM_malloc(sizeof(char) *
                                        (strlen(attr[i+1]) + 1));
                strcpy(parse->terrainFeatures->m_intersections
                       [parse->hashTable->
                        cells[hashTablePosition].objectID].XML_ID,
                       attr[i+1]);
            }
            else if (strcmp(attr[i], "ReferencePoint") == 0) {
                sscanf(attr[i+1], "%lf %lf %lf",
                       &(parse->
                         localReference.latlonalt.longitude),
                       &(parse->
                         localReference.latlonalt.latitude),
                       &(parse->
                         localReference.latlonalt.altitude));
            }
            else if (strcmp(attr[i], "CoordinateType") == 0) {
                if (strcmp(attr[i+1], "geodetic") == 0) {
                    parse->localType = GEODETIC;
                }
                else if (strcmp(attr[i+1], "cartesian") == 0) {
                    parse->localType = JGIS;
                }
                else {
                    ERROR_ReportError("Unknown coordinateType");
                }
            }
        }

        parse->terrainFeatures->m_intersections
        [parse->hashTable->
         cells[hashTablePosition].objectID].ID =
             parse->hashTable->
             cells[hashTablePosition].objectID;
        parse->terrainFeatures->m_intersections
        [parse->hashTable->
         cells[hashTablePosition].objectID].entranceType = ENTRANCE_NONE;
        parse->terrainFeatures->m_intersections
        [parse->hashTable->
         cells[hashTablePosition].objectID].park = INVALID_PARK_ID;
        parse->terrainFeatures->m_intersections
        [parse->hashTable->
         cells[hashTablePosition].objectID].station = INVALID_STATION_ID;

        //Add the intersection structure to the object stack
        parse->stack.
        objectStack[parse->stack.depth] =
            &(parse->terrainFeatures->m_intersections
              [parse->hashTable->
               cells[hashTablePosition].objectID]);
        parse->hashTable->
        cells[hashTablePosition].objectState = COMPLETE;

        parse->numSignalGroups = 0;
        parse->numTrafficLights = 0;

        parse->terrainFeatures->m_numIntersections++;
    }
    else if (strcmp(el, "location") == 0) {
        if ((parse->stack.depth < 2) ||
                (parse->stack.
                 tagStack[parse->stack.depth - 1]
                 != INTERSECTION))
        {
            ERROR_ReportError("Error: location tag encountered at incorrect "
                              "position");
        }
        if (parse->prevTag != INVALID_TAG)
        {
            ERROR_ReportError("Error: malformed intersection object");
        }
        //Add the object tag to the tag stack
        parse->stack.
        tagStack[parse->stack.depth] = LOCATION;
        //Reset the data buffer
        parse->dataBuf[0] = 0;
        parse->stack.
        objectStack[parse->stack.depth] = NULL;
        //printf("location\n");
    }
    else if (strcmp(el, "synchronizedSignals") == 0) {
        if ((parse->stack.depth < 2) ||
                (parse->stack.
                 tagStack[parse->stack.depth - 1]
                 != INTERSECTION))
        {
            ERROR_ReportError("Error: synchronizedSignals tag encountered at "
                              "incorrect position");
        }
        if ((parse->prevTag != LOCATION) &&
                (parse->prevTag != SYNC_SIGNAL))
        {
            ERROR_ReportError("Error: malformed intersection object");
        }
        //Add the object tag to the tag stack
        parse->stack.
        tagStack[parse->stack.depth] = SYNC_SIGNAL;
        parse->stack.
        objectStack[parse->stack.depth] = NULL;
        //printf("synchronizedSignals\n");
    }
    else if (strcmp(el, "reference") == 0) {
        if ((parse->stack.depth < 3) ||
                ((parse->stack.
                  tagStack[parse->stack.depth - 1]
                  != SYNC_SIGNAL) &&
                 (parse->stack.
                  tagStack[parse->stack.depth - 1]
                  != OBJECTS)))
        {
            ERROR_ReportError("Error: reference tag encountered at incorrect "
                              "position");
        }
        //Add the object tag to the tag stack
        parse->stack.
        tagStack[parse->stack.depth] = REFERENCE;
        //Reset the data buffer
        parse->dataBuf[0] = 0;
        parse->stack.
        objectStack[parse->stack.depth] = NULL;
        //printf("reference\n");

    }
    else if (strcmp(el, "objects") == 0) {
        if ((parse->stack.depth < 2) ||
                (parse->stack.
                 tagStack[parse->stack.depth - 1]
                 != INTERSECTION))
        {
            ERROR_ReportError("Error: objects tag encountered at incorrect "
                              "position");
        }
        if ((parse->prevTag != LOCATION) &&
                (parse->prevTag != SYNC_SIGNAL)){
            ERROR_ReportError("Error: malformed intersection object");

        }
        //Add the object tag to the tag stack
        parse->stack.
        tagStack[parse->stack.depth] = OBJECTS;
        parse->stack.
        objectStack[parse->stack.depth] = NULL;
        //printf("objects\n");
    }
    else {
        ERROR_ReportError("Error: Unkown tag encountered");
    }
    //Increment stack depth
    parse->stack.depth++;
}

/*! \fn static
  void XMLCALL HandleEndTag(void *data,
  const char *el)
  \brief End tag handler called by expat during second pass
  over the XML files - this is where most of the processing happenss
  \param data pointer to the TerrainFeaturesParseStruct structure
  \param el end tag string
*/
static
void XMLCALL HandleEndTag(void *data,
                          const char *el)
{
    TerrainFeaturesParseStruct *parse = (TerrainFeaturesParseStruct *)data;
    int hashTablePosition;
    char errorBuf[512];
    //Temporary data structures used while parsing
    FeatureFace *tempFace;
    Building* tempBld;
    Foliage* tempFoliage;
    RoadSegment* tempRoadSeg;
    RoadSegment* prevRoadSeg;
    Intersection *tempIntersection;
    RoadSegmentID *tempRoadIDs;
    Park* tempPark;
    Station* tempStation;
    Coordinates tempCoordinates;

    memset(&tempCoordinates, 0, sizeof(Coordinates));

    //Parse in the coordinates for most objects
    if ((((parse->stack.
            tagStack[parse->stack.depth-1] == FIRST_NODE) ||
            (parse->stack.
             tagStack[parse->stack.depth-1] == LAST_NODE)) &&
            (parse->stack.tagStack
             [parse->stack.depth-2] == STREET_SEGMENT)) ||
            ((parse->stack.
              tagStack[parse->stack.depth-1] == POSITION) &&
             ((parse->stack.tagStack
               [parse->stack.depth-2] == PARK) ||
              (parse->stack.tagStack
               [parse->stack.depth-2] == STATION))))
    {
        parseCoordinates(parse,
                         parse->
                         coordinateList[parse->numCoordinates-1]);
        parse->prevTag = parse->stack.
                         tagStack[parse->stack.depth-1];
    }
    // special parsing for building coordinates
    else if ((parse->stack.tagStack[parse->stack.depth-1] == POSITION)
             && (parse->stack.tagStack[parse->stack.depth-2] == FACE))
    {
        parseCoordinatesBuilding(parse,
                                 parse->
                                 coordinateList[parse->numCoordinates-1]);
        parse->prevTag = parse->stack.
                         tagStack[parse->stack.depth-1];
    }
    else if ((parse->stack.
              tagStack[parse->stack.depth-1] == POSITION) &&
             (parse->stack.tagStack
              [parse->stack.depth-2] == REGION))
    {
        if ((parse->localType == JGIS) ||
                ((parse->localType ==
                  INVALID_COORDINATE_TYPE) &&
                 (parse->globalType == JGIS)))
        {
            sscanf(parse->dataBuf, "%lf %lf %lf",
                   &(tempCoordinates.common.c1),
                   &(tempCoordinates.common.c2),
                   &(tempCoordinates.common.c3));
        }
        else if ((parse->localType == GEODETIC) ||
                 ((parse->localType ==
                   INVALID_COORDINATE_TYPE) &&
                  (parse->globalType == GEODETIC)))
        {
            sscanf(parse->dataBuf, "%lf %lf %lf",
                   &(tempCoordinates.latlonalt.longitude),
                   &(tempCoordinates.latlonalt.latitude),
                   &(tempCoordinates.latlonalt.altitude));
        }
        if (parse->southWestSet == FALSE) {
            parse->terrainFeatures->m_southWest.common.c1 =
                tempCoordinates.common.c1;
            parse->terrainFeatures->m_southWest.common.c2 =
                tempCoordinates.common.c2;
            parse->terrainFeatures->m_southWest.common.c3 =
                tempCoordinates.common.c3;

            parse->southWestSet = TRUE;
        }
        else {
            if (parse->terrainFeatures->m_southWest.common.c1 >
                    tempCoordinates.common.c1)
            {
                parse->terrainFeatures->m_southWest.common.c1 =
                    tempCoordinates.common.c1;
            }
            if (parse->terrainFeatures->m_southWest.common.c2 >
                    tempCoordinates.common.c2)
            {
                parse->terrainFeatures->m_southWest.common.c2 =
                    tempCoordinates.common.c2;
            }
            if (parse->terrainFeatures->m_southWest.common.c3 >
                    tempCoordinates.common.c3)
            {
                parse->terrainFeatures->m_southWest.common.c3 =
                    tempCoordinates.common.c3;
            }
        }
        parse->prevTag = parse->stack.
                         tagStack[parse->stack.depth-1];
    }
    //Parse in the coordinates for the node object
    else if ((parse->stack.
              tagStack[parse->stack.depth-1] == NODE) &&
             (parse->stack.tagStack
              [parse->stack.depth-2] == STREET_SEGMENT))
    {
        parseCoordinates(parse,
                         parse->
                         coordinateList[parse->numCoordinates-1]);
        tempRoadSeg = (RoadSegment *)parse->stack.
                      objectStack[parse->stack.depth-2];
        //If the street segment has any related street segments,
        //copy in the coordinates of the previous street segment
        //from the temporary coordinates array and reset the array
        if (tempRoadSeg->roadSegmentVar != NULL) {
            prevRoadSeg = ((TerrainFeaturesRoadLinks *)tempRoadSeg->
                           roadSegmentVar)->prevRoad;
            if (((TerrainFeaturesRoadLinks *)prevRoadSeg->roadSegmentVar)->
                    isSecondLast == TRUE)
            {
                parse->prevTag = FIRST_NODE;
                //take list of coordinates and convert it into street
                //segment's array
                prevRoadSeg->vertices =
                    (Coordinates *)MEM_malloc(parse->numCoordinates
                                              * sizeof(Coordinates));
                prevRoadSeg->num_vertices = parse->numCoordinates;
                memcpy(prevRoadSeg->vertices,
                       parse->coordinateList,
                       sizeof(Coordinates) * parse->numCoordinates);
                memcpy(parse->coordinateList,
                       parse->coordinateList + parse->numCoordinates - 1,
                       sizeof(Coordinates));
                parse->numCoordinates = 1;
                ((TerrainFeaturesRoadLinks *)prevRoadSeg->roadSegmentVar)->
                isSecondLast = FALSE;
            }
            else {
                parse->prevTag = parse->stack.
                                 tagStack[parse->stack.depth-1];
            }
        }
        else {
            parse->prevTag = parse->stack.
                             tagStack[parse->stack.depth-1];
        }
    }
    //Parse the width of the street segment on the stack
    else if ((parse->stack.
              tagStack[parse->stack.depth-1] == WIDTH) &&
             (parse->stack.tagStack
              [parse->stack.depth-2] == STREET_SEGMENT))
    {
        sscanf(parse->dataBuf, "%lf",
               &(((RoadSegment *)parse->stack.
                  objectStack[parse->stack.depth-2])->
                 width));
        //write same width to all street segments that
        //are related to the current street segment on the stack
        tempRoadSeg = (RoadSegment *)parse->stack.
                      objectStack[parse->stack.depth-2];
        if (tempRoadSeg->roadSegmentVar != NULL) {
            tempRoadSeg = ((TerrainFeaturesRoadLinks *)tempRoadSeg->
                           roadSegmentVar)->prevRoad;
            while (tempRoadSeg != NULL) {
                tempRoadSeg->width =
                    ((TerrainFeaturesRoadLinks *)tempRoadSeg->
                     roadSegmentVar)->nextRoad->width;
                tempRoadSeg =
                    ((TerrainFeaturesRoadLinks *)tempRoadSeg->
                     roadSegmentVar)->prevRoad;
            }
        }

        parse->prevTag = parse->stack.
                         tagStack[parse->stack.depth-1];
    }
    //Parse the location of the intersection on the stack
    else if (parse->stack.
             tagStack[parse->stack.depth-1] == LOCATION)
    {
        parseCoordinates(parse,
                         ((Intersection *)parse->stack.
                          objectStack
                          [parse->stack.depth-2])->
                         position);

        parse->prevTag = parse->stack.
                         tagStack[parse->stack.depth-1];
    }
    else if (parse->stack.
             tagStack[parse->stack.depth-1] == SYNC_SIGNAL)
    {
        parse->prevTag = parse->
                         stack.tagStack[parse->stack.depth-1];
        parse->numSignalGroups++;
    }
    else if (parse->stack.tagStack
             [parse->stack.depth-1] == STREET_SEGMENT)
    {
        parse->prevTag = INVALID_TAG;
        if (parse->numCoordinates == 0) {
            ERROR_ReportError("Street segment missing nodes");
        }
        //Copy the coordinates of the street segment
        //from the temporary coordinates array
        tempRoadSeg = (RoadSegment *)parse->stack.
                      objectStack[parse->stack.depth-1];
        tempRoadSeg->vertices =
            (Coordinates *)MEM_malloc(parse->numCoordinates
                                      * sizeof(Coordinates));
        tempRoadSeg->num_vertices = parse->numCoordinates;
        memcpy(tempRoadSeg->vertices,
               parse->coordinateList,
               sizeof(Coordinates) * parse->numCoordinates);
        parse->numCoordinates = 0;
        parse->localReference.latlonalt.longitude = 360.0;
        parse->localReference.latlonalt.latitude = 360.0;
        parse->localType = INVALID_COORDINATE_TYPE;
    }
    else if (parse->stack.tagStack
             [parse->stack.depth-1] == PARK)
    {
        parse->prevTag = INVALID_TAG;
        if (parse->numCoordinates == 0) {
            ERROR_ReportError("Park missing coordinates");
        }
        //Copy the coordinates and exit intersection
        //IDs of the park on the stack from the corresponding
        //temporary structures
        tempPark = (Park *)parse->stack.
                   objectStack[parse->stack.depth-1];
        tempPark->vertices =
            (Coordinates *)MEM_malloc(parse->numCoordinates
                                      * sizeof(Coordinates));
        tempPark->num_vertices = parse->numCoordinates;
        memcpy(tempPark->vertices,
               parse->coordinateList,
               sizeof(Coordinates) * parse->numCoordinates);
        tempPark->exitIntersections =
            (IntersectionID *)MEM_malloc(
                parse->numCoordinates *
                sizeof(IntersectionID));
        memcpy(tempPark->exitIntersections,
               parse->intIDs,
               sizeof(IntersectionID) *
               parse->numCoordinates);

        tempPark->parkStationVar = NULL;
        parse->numCoordinates = 0;
        parse->localReference.latlonalt.longitude = 360.0;
        parse->localReference.latlonalt.latitude = 360.0;
        parse->localType = INVALID_COORDINATE_TYPE;
    }
    else if (parse->stack.tagStack
             [parse->stack.depth-1] == REGION)
    {
        parse->prevTag = INVALID_TAG;
        parse->localType = INVALID_COORDINATE_TYPE;
    }
    else if (parse->stack.tagStack
             [parse->stack.depth-1] == STATION)
    {
        parse->prevTag = INVALID_TAG;
        if (parse->numCoordinates == 0) {
            ERROR_ReportError("Station missing coordinates");

        }
        //Copy the coordinates and exit intersection
        //IDs of the station on the stack from the corresponding
        //temporary structures
        tempStation = (Station *)parse->stack.
                      objectStack[parse->stack.depth-1];
        tempStation->vertices =
            (Coordinates *)MEM_malloc(parse->numCoordinates
                                      * sizeof(Coordinates));
        tempStation->num_vertices = parse->numCoordinates;
        memcpy(tempStation->vertices,
               parse->coordinateList,
               sizeof(Coordinates) * parse->numCoordinates);
        tempStation->exitIntersections =
            (IntersectionID *)MEM_malloc(
                parse->numCoordinates
                * sizeof(IntersectionID));
        memcpy(tempStation->exitIntersections,
               parse->intIDs,
               sizeof(IntersectionID) *
               parse->numCoordinates);
        parse->numCoordinates = 0;
        parse->localReference.latlonalt.longitude = 360.0;
        parse->localReference.latlonalt.latitude = 360.0;
        parse->localType = INVALID_COORDINATE_TYPE;
    }
    //Copy in the object IDs of all objects
    //that meet at the intersection on the stack
    else if (parse->stack.
             tagStack[parse->stack.depth-1] == OBJECTS)
    {
        parse->prevTag = parse->stack.
                         tagStack[parse->stack.depth-1];
        if (parse->numRoadIds == 0) {
            ERROR_ReportError("Intersection missing roadsegments");
        }
        tempIntersection = (Intersection *)parse->stack.
                           objectStack[parse->stack.depth-2];
        tempIntersection->roadSegments =
            (RoadSegmentID *)MEM_malloc(parse->numRoadIds *
                                        sizeof(RoadSegmentID));
        tempIntersection->num_road_segments = parse->numRoadIds;
        memcpy(tempIntersection->roadSegments,
               parse->roadIDs,
               sizeof(RoadSegmentID) * parse->numRoadIds);
        parse->numRoadIds = 0;

        // could do this at end tag of intersection instead,
        // which should be the next end tag
        tempIntersection->numSignalGroups = parse->numSignalGroups;

        if (tempIntersection->numSignalGroups != 0)
        {
            if (parse->numTrafficLights != tempIntersection->num_road_segments)
            {
                sprintf(errorBuf, "Intersection %s: "
                        "the total number of references under "
                        "<synchronizedSignals> does not mach the number "
                        "of references under <objects>\n", tempIntersection->XML_ID);
                ERROR_ReportError(errorBuf);
            }

            // save traffic light data, which cannot be completed until
            // after some road segments are broken up
            tempIntersection->intersectionVar =
                (TrafficLight*) MEM_malloc(parse->numTrafficLights *
                                           sizeof(TrafficLight));
            memcpy(tempIntersection->intersectionVar,
                   parse->trafficLights,
                   sizeof(TrafficLight) * parse->numTrafficLights);

            tempIntersection->hasTrafficSignal = TRUE;
        }
        else
        {
            tempIntersection->intersectionVar = NULL;
            tempIntersection->hasTrafficSignal = FALSE;
        }
    }
    else if (parse->stack.
             tagStack[parse->stack.depth-1] == INTERSECTION)
    {
        parse->prevTag = INVALID_TAG;
        parse->localReference.latlonalt.longitude = 360.0;
        parse->localReference.latlonalt.latitude = 360.0;
        parse->localType = INVALID_COORDINATE_TYPE;
    }
    //Parse the object IDs of all the objects
    //that meet at the intersection on the stack. Also set the
    //intersection type based on the objects that lie at the
    //intersection
    else if (parse->stack.
             tagStack[parse->stack.depth-1] == REFERENCE)
    {
        if (parse->stack.
                tagStack[parse->stack.depth-2] == OBJECTS)
        {
            hashTablePosition = Find(parse,
                                     parse->dataBuf);
            if (parse->hashTable->
                    cells[hashTablePosition].entryState == EMPTY)
            {
                sprintf(errorBuf, "Undefined ID reference found in "
                        "intersection definition: %s",
                        parse->dataBuf);
                ERROR_ReportError(errorBuf);
            }
            else if (parse->hashTable->
                     cells[hashTablePosition].objectType == PARK)
            {
                if (((Intersection *)parse->stack.
                        objectStack[parse->stack.depth-3])->
                        entranceType != ENTRANCE_STATION)
                {
                    ((Intersection *)parse->stack.
                     objectStack[parse->stack.depth-3])->
                    entranceType = ENTRANCE_PARK;
                    ((Intersection *)parse->stack.
                     objectStack[parse->stack.depth-3])->
                    park = parse->hashTable->
                           cells[hashTablePosition].objectID;
                }
                else {
                    ERROR_ReportError("Intersections with both park(s) and "
                                      "station(s) found");
                }
            }
            else if (parse->hashTable->
                     cells[hashTablePosition].objectType == STATION)
            {
                if (((Intersection *)parse->stack.
                        objectStack[parse->stack.depth-3])->
                        entranceType != ENTRANCE_PARK)
                {
                    ((Intersection *)parse->stack.
                     objectStack[parse->stack.depth-3])->
                    entranceType = ENTRANCE_STATION;
                    ((Intersection *)parse->stack.
                     objectStack[parse->stack.depth-3])->
                    park = parse->hashTable->
                           cells[hashTablePosition].objectID;
                }
                else {
                    ERROR_ReportError("Intersections with both park(s) and "
                                      "station(s) found");
                }
            }
            else if (parse->hashTable->
                     cells[hashTablePosition].objectType == STREET_SEGMENT) {
                if (parse->numRoadIds ==
                        parse->maxRoadIds)
                {
                    parse->maxRoadIds =
                        parse->maxRoadIds*2;
                    tempRoadIDs = parse->roadIDs;
                    parse->roadIDs = (RoadSegmentID *)
                                     MEM_malloc(sizeof(RoadSegmentID) *
                                                parse->maxRoadIds);
                    memcpy(parse->roadIDs,
                           tempRoadIDs,
                           sizeof(RoadSegmentID) *
                           parse->numRoadIds);
                    MEM_free(tempRoadIDs);
                }
                parse->roadIDs[parse->
                               numRoadIds] =
                                   parse->hashTable->
                                   cells[hashTablePosition].objectID;
                parse->numRoadIds++;
            }
        }
        else if (parse->stack.
                 tagStack[parse->stack.depth-2] == SYNC_SIGNAL)
        {
            hashTablePosition = Find(parse,
                                     parse->dataBuf);
            if (parse->hashTable->
                    cells[hashTablePosition].objectType == STREET_SEGMENT) {
                if (parse->numTrafficLights ==      // trafficIds
                        parse->maxTrafficLights)
                {
                    parse->maxTrafficLights =
                        parse->maxTrafficLights * 2;
                    TrafficLight* tempTrafficLights = parse->trafficLights;
                    parse->trafficLights = (TrafficLight *)
                                           MEM_malloc(sizeof(TrafficLight) *
                                                      parse->maxTrafficLights);
                    memcpy(parse->trafficLights,
                           tempTrafficLights,
                           sizeof(TrafficLight) *
                           parse->numTrafficLights);
                    MEM_free(tempTrafficLights);
                }
                parse->trafficLights[parse->
                                     numTrafficLights].roadID =
                                         parse->hashTable->
                                         cells[hashTablePosition].objectID;

                parse->trafficLights[parse->
                                     numTrafficLights].signalGroup =
                                         parse->numSignalGroups;

                parse->numTrafficLights++;
            }
        }
    }
    //Parse the coordinates of the representative
    //corrseponding to the park or station on the stack
    else if (parse->stack.
             tagStack[parse->stack.depth-1] ==
             REPRESENTATIVE)
    {
        if (parse->stack.tagStack
                [parse->stack.depth-2] == PARK)
        {
            parseCoordinates(parse,
                             ((Park *)parse->stack.
                              objectStack
                              [parse->stack.depth-2])->
                             representative);
        }
        else if (parse->stack.tagStack
                 [parse->stack.depth-2] == STATION)
        {
            parseCoordinates(parse,
                             ((Station *)parse->stack.
                              objectStack
                              [parse->stack.depth-2])->
                             representative);
        }

        parse->prevTag = parse->stack.
                         tagStack[parse->stack.depth-1];
    }
    //Parse the thickness of the building face on the stack
    else if ((parse->stack.
              tagStack[parse->stack.depth-1] == THICKNESS) &&
             (parse->stack.tagStack
              [parse->stack.depth-2] == FACE))
    {
        sscanf(parse->dataBuf, "%lf",
               &(((FeatureFace *)parse->stack.
                  objectStack[parse->stack.depth-2])->
                 thickness));
        parse->prevTag = parse->stack.
                         tagStack[parse->stack.depth-1];
    }
    //Copy the coordinates of the building face on the stack
    //from the temporary coordinates array
    else if (parse->stack.
             tagStack[parse->stack.depth-1] == FACE)
    {
        parse->prevTag = INVALID_TAG;
        if (parse->numCoordinates < 3) {
            ERROR_ReportError("Face missing coordinates");
        }
        tempFace = (FeatureFace *)parse->stack.
                   objectStack[parse->stack.depth-1];
        tempFace->vertices =
            (Coordinates *)MEM_malloc(parse->numCoordinates
                                      * sizeof(Coordinates));
        tempFace->num_vertices = parse->numCoordinates;
        memcpy(tempFace->vertices,
               parse->coordinateList,
               sizeof(Coordinates) * parse->numCoordinates);
        parse->numCoordinates = 0;
        parse->localReference.latlonalt.longitude = 360.0;
        parse->localReference.latlonalt.latitude = 360.0;
        parse->localType = INVALID_COORDINATE_TYPE;
    }
    //copy the building faces of the building on the stack
    //from the temporary face array
    else if (parse->stack.
             tagStack[parse->stack.depth-1] == BUILDING)
    {
        if (parse->numFaces < 4) {
            ERROR_ReportError("Building missing faces");
        }
        tempBld = (Building *)parse->stack.
                  objectStack[parse->stack.depth-1];
        tempBld->faces =
            (FeatureFace *)MEM_malloc(parse->numFaces
                                      * sizeof(FeatureFace));
        tempBld->num_faces = parse->numFaces;
        memcpy(tempBld->faces,
               parse->faces,
               sizeof(FeatureFace) * parse->numFaces);
        parse->numFaces = 0;
        parse->buildingReference.latlonalt.longitude = 360.0;
        parse->buildingReference.latlonalt.latitude = 360.0;
        parse->buildingType = INVALID_COORDINATE_TYPE;
    }
    //copy the foliage faces of the foliage on the stack
    //from the temporary face array
    else if (parse->stack.
             tagStack[parse->stack.depth-1] == FOLIAGE)
    {
        if (parse->numFaces < 4) {
            ERROR_ReportError("Foliage missing faces");
        }
        tempFoliage = (Foliage *)parse->stack.
                      objectStack[parse->stack.depth-1];
        tempFoliage->faces =
            (FeatureFace *)MEM_malloc(parse->numFaces
                                      * sizeof(FeatureFace));
        tempFoliage->num_faces = parse->numFaces;
        memcpy(tempFoliage->faces,
               parse->faces,
               sizeof(FeatureFace) * parse->numFaces);
        parse->numFaces = 0;
        parse->foliageReference.latlonalt.longitude = 360.0;
        parse->foliageReference.latlonalt.latitude = 360.0;
        parse->foliageType = INVALID_COORDINATE_TYPE;
    }
    //Decrement stack depth
    parse->stack.depth--;
}

/*! \fn static
  void XMLCALL HandleData(void *userData,
  const XML_Char *s,
  int len)
  \brief Character data handler called by expat during second pass
  when it encounters character data between start and end tags
  \param userData pointer to the TerrainFeaturesParseStruct structure
  \param s String containing character data read so far
  \param len length of the string s
*/
static
void XMLCALL HandleData(void *userData,
                        const XML_Char *s,
                        int len)
{
    TerrainFeaturesParseStruct *parse =
        (TerrainFeaturesParseStruct *)userData;
    char buf[512];

    strncpy(buf, s, len);
    buf[len] = 0;

    //For all relevant objects, append the character data string
    //to the data buffer
    if ((parse->stack.
            tagStack[parse->stack.depth-1] == FIRST_NODE) ||
            (parse->stack.
             tagStack[parse->stack.depth-1] == NODE) ||
            (parse->stack.
             tagStack[parse->stack.depth-1] == LAST_NODE) ||
            (parse->stack.
             tagStack[parse->stack.depth-1] == WIDTH) ||
            (parse->stack.
             tagStack[parse->stack.depth-1] == LOCATION) ||
            (parse->stack.
             tagStack[parse->stack.depth-1] == REFERENCE) ||
            (parse->stack.
             tagStack[parse->stack.depth-1] == POSITION) ||
            (parse->stack.
             tagStack[parse->stack.depth-1] == REPRESENTATIVE)  ||
            (parse->stack.
             tagStack[parse->stack.depth-1] == THICKNESS))
    {
        strcat(parse->dataBuf, buf);
    }
}

// non-static functions

/*! \fn void ParseQualNetUrbanTerrain(TerrainData*
  terrainData,
  NodeInput *nodeInput)
  \brief Function called to parse terrain feature data from the
  XML files
  \param terrainData pointer to a TerrainData structure
  \param nodeInput Pointer to the qualnet config input structure
  qualnet config file
*/
void ParseQualNetUrbanTerrain(TerrainData*             terrainData,
                              QualNetUrbanTerrainData* terrainFeatures,
                              NodeInput*               nodeInput,
                              bool                     masterProcess)
{
    FILE *input;
    TerrainFeaturesParseStruct *parse;
    int count;
    char buff[TERRAIN_FEATURES_PARSE_BUFF_SIZE];
    //Array of strings containing the files names of all the XML
    //files to be parsed
    char **fileName = NULL;
    XML_Parser parser;
    char errorBuf[512];
    BOOL wasFound;
    int i;
    int j;
    //Temporary structures used while parsing
    RoadSegment *tempRoadSeg;
    RoadSegmentID *tempRoadSegIDs;

    int numFiles = 0;
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];

    IO_ReadString(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "TERRAIN-FEATURES-SOURCE",
        &wasFound,
        buf);

    if (wasFound) {
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
                IO_ReadString(
                    ANY_NODEID,
                    ANY_ADDRESS,
                    nodeInput,
                    "TERRAIN-FEATURES-FILENAME",
                    &wasFound,
                    buf2);

                if (wasFound)
                {
                    ERROR_ReportError("TERRAIN-FEATURES-FILELIST and "
                                      "TERRAIN-FEATURES-FILENAME should not "
                                      "both be specified.\n");
                }

                FILE* file = fopen(buf, "r");
                if (file == NULL)
                {
                    sprintf(errorBuf, "Could not open "
                            "TERRAIN-FEATURES-FILELIST: %s\n",
                            buf);
                    ERROR_ReportError(errorBuf);
                }

                while (fgets(buf2, MAX_STRING_LENGTH, file) != NULL)
                {
                    numFiles++;
                }
                fclose(file);

                if (numFiles == 0) {
                    return;
                }

                fileName = (char **)MEM_malloc(sizeof(char *) * numFiles);

                file = fopen(buf, "r");
                if (file == NULL)
                {
                    sprintf(errorBuf, "Could not open "
                            "TERRAIN-FEATURES-FILELIST: %s\n",
                            buf);
                    ERROR_ReportError(errorBuf);
                }

                char* p;
                for (count = 0; count < numFiles; count++)
                {
                    fileName[count] = (char *)MEM_malloc(sizeof(char) * 256);
                    if (fgets(fileName[count], MAX_STRING_LENGTH, file) == NULL) { break; }

                    if ((p = strchr(fileName[count], '\n')) != NULL)
                    {
                        *p = '\0';
                    }
                }
                fclose(file);
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
                }
                if (numFiles == 0) {
                    return;
                }

                fileName = (char **)MEM_malloc(sizeof(char *) * numFiles);
                for (count = 0; count < numFiles; count++) {
                    fileName[count] = (char *)MEM_malloc(sizeof(char) * 256);

                    BOOL fallBackToGlobal = FALSE;
                    if (count == 0) { fallBackToGlobal = TRUE; }

                    IO_ReadStringInstance(ANY_NODEID,
                                          ANY_ADDRESS,
                                          nodeInput,
                                          "TERRAIN-FEATURES-FILENAME",
                                          count,
                                          fallBackToGlobal,
                                          &wasFound,
                                          fileName[count]);

                    if (!wasFound) {
                        sprintf(errorBuf, "TERRAIN-FEATURES-FILENAME[%d] "
                                "missing from config file\n", count);
                        ERROR_ReportError(errorBuf);
                    }
                }
            }
        }
#ifdef CTDB7_INTERFACE
        else if (strcmp(buf, "CTDB7") == 0) {
            char outputPath[FILENAME_MAX];
#ifdef USE_MPI
            if (masterProcess)
            {
#endif
                CtdbFormatData* data =
                    CtdbFormatInitialize(
                        nodeInput,
                        (terrainData->getElevationModel() != "CTDB7"));

                CtdbFormatSearchBlds(data);

                strcpy(outputPath, data->outputPath);
                numFiles = data->numFiles;
                CtdbFormatFinalize(data);
#ifdef USE_MPI
            }
            MPI_Bcast (outputPath, FILENAME_MAX, MPI_BYTE, 0, MPI_COMM_WORLD);
            MPI_Bcast (&numFiles, sizeof(int), MPI_BYTE, 0, MPI_COMM_WORLD);
#endif

            if (numFiles > 0)
            {
                fileName = (char **)MEM_malloc(sizeof(char *) * numFiles);
                for (count = 0; count < numFiles; count++) {
                    fileName[count] = (char *)MEM_malloc(sizeof(char) * 256);
                    sprintf(fileName[count], "%s%d.xml",
                            outputPath,
                            count+1);
                }
            }
            else
            {
                return;
            }
        }
#endif // CTDB7_INTERFACE
#ifdef CTDB8_INTERFACE
        else if (strcmp(buf, "CTDB8") == 0) {
            char outputPath[FILENAME_MAX];
#ifdef USE_MPI
            if (masterProcess)
            {
#endif
                CtdbFormatData* data =
                    CtdbFormatInitialize(
                        nodeInput,
                        (terrainData->getElevationModel() != "CTDB8"));

                CtdbFormatSearchBlds(data);
                CtdbFormatSearchRds(data);

                strcpy(outputPath, data->outputPath);
                numFiles = data->numFiles;
                CtdbFormatFinalize(data);
#ifdef USE_MPI
            }
            MPI_Bcast (outputPath, FILENAME_MAX, MPI_BYTE, 0, MPI_COMM_WORLD);
            MPI_Bcast (&numFiles, sizeof(int), MPI_BYTE, 0, MPI_COMM_WORLD);
#endif

            if (numFiles > 0)
            {
                fileName = (char **)MEM_malloc(sizeof(char *) * numFiles);
                for (count = 0; count < numFiles; count++) {
                    fileName[count] = (char *)MEM_malloc(sizeof(char) * 256);
                    sprintf(fileName[count], "%s%d.xml",
                            outputPath,
                            count+1);
                }
            }
            else
            {
                return;
            }
        }
#endif // CTDB8_INTERFACE
//#ifdef ESRI_SHP_INTERFACE
        else if (strcmp(buf, "SHAPEFILE") == 0) {
#ifdef USE_MPI
            if (masterProcess)
            {
#endif
                EsriShpInit(terrainData, 
                                 nodeInput, 
                                 numFiles, 
                                 fileName, 
                                 masterProcess);
#ifdef USE_MPI
            }
#endif
        }
//#endif // ESRI_SHP_INTERFACE
        else {
            ERROR_ReportError("Unknown TERRAIN-FEATURES-SOURCE\n");
        }
    }
    else {
        return;
    }

    parser = XML_ParserCreate(NULL);
    if (!parser) {
        ERROR_ReportError("Couldn't allocate memory for parser");
    }

    //Setup all the relevant data structures
    parse = (TerrainFeaturesParseStruct *)
            MEM_malloc(sizeof(TerrainFeaturesParseStruct));
    parse->coordinateSystemType = terrainData->getCoordinateSystem();
    parse->terrainData = terrainData;

    parse->terrainFeatures = terrainFeatures;
    parse->stack.depth = 0;
    parse->prevTag = INVALID_TAG;
    InitializeTable(parse,
                    TERRAIN_FEATURES_INIT_HASH_TABLE_SIZE);
    parse->maxRoadSegments = 0;
    parse->maxBuildings = 0;
    parse->maxFoliage = 0;
    parse->maxIntersections = 0;
    parse->maxParks = 0;
    parse->maxStations = 0;
    parse->maxClouds = 0;
    parse->maxRailroads = 0;
    parse->maxCoordinates = TERRAIN_FEATURES_INIT_ARRAY_SIZE;
    parse->maxFaces = TERRAIN_FEATURES_INIT_ARRAY_SIZE;
    parse->maxFeatureFaces = TERRAIN_FEATURES_INIT_ARRAY_SIZE;
    parse->maxRoadIds = TERRAIN_FEATURES_INIT_ARRAY_SIZE;
    parse->maxTrafficLights = TERRAIN_FEATURES_INIT_ARRAY_SIZE;
    parse->maxIntIds = 0;
    parse->maxStreetSegObjects = 0;
    parse->southWestSet = FALSE;
    parse->globalSet = FALSE;

    parse->localReference.latlonalt.longitude = 360.0;
    parse->localReference.latlonalt.latitude = 360.0;
    parse->buildingReference.latlonalt.longitude = 360.0;
    parse->buildingReference.latlonalt.latitude = 360.0;
    parse->localReference.type = GEODETIC;
    parse->globalReference.type = GEODETIC;
    parse->buildingReference.type = GEODETIC;
    parse->foliageReference.type = GEODETIC;
    parse->localType = INVALID_COORDINATE_TYPE;
    parse->buildingType = INVALID_COORDINATE_TYPE;
    parse->foliageType = INVALID_COORDINATE_TYPE;
    parse->siteID = NULL;

    // Read parameter global foliated state
    // TODO:  Move to prop_foliage.cpp
    IO_ReadString(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "FOLIAGE-FOLIATED-STATE",
        &wasFound,
        buf);

    if (wasFound && !strcmp(buf, "NO")) {
        parse->terrainFeatures->m_foliatedState = OUT_OF_LEAF;
    }
    else {
        parse->terrainFeatures->m_foliatedState = IN_LEAF;
    }

    // Read parameter for putting terrain features on the ground
    IO_ReadString(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "TERRAIN-FEATURES-SET-FEATURES-TO-GROUND",
        &wasFound,
        buf);

    if (wasFound && !strcmp(buf, "NO")) {
        parse->terrainFeatures->m_setFeaturesToGround = false;
    }
    else {
        parse->terrainFeatures->m_setFeaturesToGround = true;
    }

    // Not quite sure what this setting does
    IO_ReadBool(ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "TERRAIN-FEATURES-SUBTRACT-TERRAIN-ELEVATION",
                &wasFound,
                &parse->subtractTerrain);
    if (!wasFound) {
        parse->subtractTerrain = FALSE;
    }

    //Perform the first pass over the files
    for (i = 0; i < numFiles; i++) {
        if (DEBUG) {
            printf("Pass 1: Parsing file %s\n", fileName[i]);
        }
        input = fopen(fileName[i], "r");

        if (!input) {
            sprintf(errorBuf, "Terrain Feature file missing %s", fileName[i]);
            ERROR_ReportError(errorBuf);
        }
        //Set the tag and data handlers
        XML_SetElementHandler(parser, InitStartTag, InitEndTag);
        XML_SetUserData(parser, parse);
        for (;;) {
            int done;
            int len;

            len = (int)fread(buff, 1, TERRAIN_FEATURES_PARSE_BUFF_SIZE,
                             input);
            if (ferror(input)) {
                sprintf(errorBuf, "Error while reading terrain feature file %s",
                        fileName[i]);
                ERROR_ReportError(errorBuf);
            }
            done = feof(input);

            if (XML_Parse(parser, buff, len, done) == XML_STATUS_ERROR) {
                sprintf(errorBuf, "Parse error at line %d while parsing "
                        "terrain feature file %s:\n%s\n",
                        XML_GetCurrentLineNumber(parser),
                        fileName[i],
                        XML_ErrorString(XML_GetErrorCode(parser)));
                ERROR_ReportError(errorBuf);

            }

            if (done)
                break;
        }
        fclose(input);
        XML_ParserReset(parser, NULL);
    }

    MEM_free(parse->siteID);

    //Setup the terrain feature structures based on
    //information gathered from the first pass
    if (parse->maxBuildings > 0) {
        parse->terrainFeatures->m_buildings = (Building *)
                                              MEM_malloc(sizeof(Building) * parse->maxBuildings);
        memset(parse->terrainFeatures->m_buildings, 0,
               sizeof(Building) * parse->maxBuildings);
    }
    if (parse->maxFoliage > 0) {
        parse->terrainFeatures->m_foliage = (Foliage *)
                                            MEM_malloc(sizeof(Foliage) * parse->maxFoliage);
        memset(parse->terrainFeatures->m_foliage, 0,
               sizeof(Foliage) * parse->maxFoliage);
    }
    if (parse->maxRoadSegments > 0) {
        parse->terrainFeatures->m_roadSegments = (RoadSegment *)
                MEM_malloc(sizeof(RoadSegment) * parse->maxRoadSegments);
    }
    if (parse->maxIntersections > 0) {
        parse->terrainFeatures->m_intersections = (Intersection *)
                MEM_malloc(sizeof(Intersection) * parse->maxIntersections);
    }
    if (parse->maxParks > 0) {
        parse->terrainFeatures->m_parks = (Park *)
                                          MEM_malloc(sizeof(Park) * parse->maxParks);
    }
    if (parse->maxStations > 0) {
        parse->terrainFeatures->m_stations = (Station *)
                                             MEM_malloc(sizeof(Station) * parse->maxStations);
    }
    if (parse->maxCoordinates > 0) {
        parse->coordinateList = (Coordinates *)
                                MEM_malloc(sizeof(Coordinates) * parse->maxCoordinates);
    }
    if (parse->maxFaces > 0) {
        parse->faces = (FeatureFace *)
                       MEM_malloc(sizeof(FeatureFace) * parse->maxFaces);
    }
    if (parse->maxFeatureFaces > 0) {
        parse->foliageFaces = (FeatureFace *)
                              MEM_malloc(sizeof(FeatureFace) * parse->maxFeatureFaces);
    }
    if (parse->maxRoadIds > 0) {
        parse->roadIDs = (RoadSegmentID *)
                         MEM_malloc(sizeof(RoadSegmentID) * parse->maxRoadIds);
    }
    if (parse->maxTrafficLights > 0) {
        parse->trafficLights = (TrafficLight*)
                               MEM_malloc(sizeof(TrafficLight) * parse->maxTrafficLights);
    }

    parse->intIDs = NULL;
    parse->numCoordinates = 0;
    parse->numFaces = 0;
    parse->numFeatureFaces = 0;
    parse->numRoadIds = 0;
    parse->numExtraStreetSegs = 0;

    //Perform the second pass over the XML files
    for (i = 0;i < numFiles; i++) {
        if (DEBUG) {
            printf("Pass 2: Parsing file %s\n", fileName[i]);
        }
        input = fopen(fileName[i], "r");

        if (!input) {
            sprintf(errorBuf, "Terrain Feature file missing %s", fileName[i]);
            ERROR_ReportError(errorBuf);
        }
        //Set the tag and data handlers
        XML_SetElementHandler(parser,
                              HandleStartTag,
                              HandleEndTag);
        XML_SetCharacterDataHandler(parser,
                                    HandleData);
        XML_SetUserData(parser, parse);
        for (;;) {
            int done;
            int len;

            len = (int)fread(buff,
                             1,
                             TERRAIN_FEATURES_PARSE_BUFF_SIZE,
                             input);
            if (ferror(input)) {
                sprintf(errorBuf, "Error reading terrain feature file %s",
                        fileName[i]);
                ERROR_ReportError(errorBuf);
            }
            done = feof(input);

            if (XML_Parse(parser, buff, len, done) == XML_STATUS_ERROR) {
                sprintf(errorBuf, "Parse error at line %d in %s:\n%s\n",
                        XML_GetCurrentLineNumber(parser),
                        fileName[i],
                        XML_ErrorString(XML_GetErrorCode(parser)));
                ERROR_ReportError(errorBuf);

            }

            if (done)
                break;
        }
        fclose(input);
        XML_ParserReset(parser, NULL);
    }

    //Done with array of terrain feature filenames
    if (fileName != NULL)
    {
        assert(numFiles > 0);

        int i;
        for (i = 0; i < numFiles; i++)
        {
            MEM_free(fileName[i]);
        }

        MEM_free(fileName);
    }

    if (parse->terrainFeatures->m_numRoadSegments !=
            parse->maxRoadSegments)
    {
        ERROR_ReportError("Incorrect number of street segments parsed during "
                          "second pass");
    }
    //Print out road segment data
    if (DEBUG) {
        printf("Region: %lf %lf %lf\n",
               parse->terrainFeatures->m_southWest.common.c1,
               parse->terrainFeatures->m_southWest.common.c2,
               parse->terrainFeatures->m_southWest.common.c3);

        for (count = 0;
                count < parse->terrainFeatures->m_numRoadSegments;
                count++)
        {
            printf("segment %d:\n\n", count);
            if (parse->terrainFeatures->m_roadSegments[count].streetName) {
                printf("\t%s ",
                       parse->terrainFeatures->m_roadSegments[count].XML_ID);
                printf("%s\n",
                       parse->terrainFeatures->m_roadSegments[count].
                       streetName);
            }
            else {
                printf("\t%s\n",
                       parse->terrainFeatures->m_roadSegments[count].XML_ID);
            }
            printf("\twidth %lf %d, %d, %d\n",
                   parse->terrainFeatures->m_roadSegments[count].width,
                   parse->terrainFeatures->m_roadSegments[count].num_vertices,
                   parse->terrainFeatures->m_roadSegments[count].
                   firstIntersection,
                   parse->terrainFeatures->m_roadSegments[count].
                   secondIntersection);

            printf("\tfirst intersection %s\n",
                   parse->terrainFeatures->
                   m_intersections[parse->terrainFeatures->
                                   m_roadSegments[count].firstIntersection].
                   XML_ID);
            printf("\tsecond intersection %s\n",
                   parse->terrainFeatures->
                   m_intersections[parse->terrainFeatures->
                                   m_roadSegments[count].secondIntersection].
                   XML_ID);
            for (i = 0;
                    i < parse->terrainFeatures->m_roadSegments[count].num_vertices;
                    i++)
            {
                printf("\tVertex %d: %lf %lf %lf\n", i,
                       parse->terrainFeatures->m_roadSegments[count].
                       vertices[i].common.c1,
                       parse->terrainFeatures->m_roadSegments[count].
                       vertices[i].common.c2,
                       parse->terrainFeatures->m_roadSegments[count].
                       vertices[i].common.c3);
            }
        }
    }
    //Correct the intersection structures for intersections that lie
    //in the middle of a set of related street segments
    for (count = 0;
            count < parse->terrainFeatures->m_numIntersections;
            count++)
    {
        for (i = 0;
                i < parse->terrainFeatures->
                m_intersections[count].num_road_segments;
                i++)
        {
            if (parse->terrainFeatures->m_roadSegments
                    [parse->terrainFeatures->
                     m_intersections[count].roadSegments[i]].roadSegmentVar != NULL)
            {
                tempRoadSeg = &(parse->terrainFeatures->m_roadSegments
                                [parse->terrainFeatures->
                                 m_intersections[count].roadSegments[i]]);
                if (tempRoadSeg->firstIntersection == parse->
                        terrainFeatures->m_intersections[count].ID)
                {
                    break;
                }
                else {
                    while (tempRoadSeg != NULL) {
                        if (tempRoadSeg->secondIntersection ==
                                parse->terrainFeatures->m_intersections[count].ID)
                        {
                            parse->terrainFeatures->m_intersections[count].
                            roadSegments[i] = tempRoadSeg->ID;
                            if (((TerrainFeaturesRoadLinks *)tempRoadSeg->
                                    roadSegmentVar)->nextRoad != NULL) {

                                // this intersection should not have traffic lights
                                // could not specify in files street segments that did not exist
                                if (parse->terrainFeatures->m_intersections[count].hasTrafficSignal == TRUE) {
                                    sprintf(errorBuf, "Intersection %s may not have traffic lights because it is an intersection where road segment %s has become split\n", parse->terrainFeatures->m_intersections[count].XML_ID, tempRoadSeg->XML_ID);
                                    ERROR_ReportError(errorBuf);
                                }

                                parse->terrainFeatures->m_intersections[count].
                                num_road_segments++;
                                tempRoadSegIDs = parse->
                                                 terrainFeatures->
                                                 m_intersections[count].roadSegments;
                                parse->terrainFeatures->m_intersections[count].
                                roadSegments =
                                    (RoadSegmentID *)MEM_malloc(
                                        sizeof(RoadSegmentID) *
                                        parse->terrainFeatures->m_intersections[count].
                                        num_road_segments);
                                memcpy(parse->terrainFeatures->
                                       m_intersections[count].roadSegments,
                                       tempRoadSegIDs,
                                       sizeof(RoadSegmentID) *
                                       (i+1));
                                // could do one copy and this one to the end instead
                                // i.e. the extra doesn't have to be number i+1
                                parse->terrainFeatures->
                                m_intersections[count].roadSegments[i+1] =
                                    ((TerrainFeaturesRoadLinks *)tempRoadSeg->
                                     roadSegmentVar)->nextRoad->ID;
                                memcpy(parse->terrainFeatures->
                                       m_intersections[count].
                                       roadSegments + i + 2,
                                       tempRoadSegIDs + i + 1,
                                       sizeof(RoadSegmentID) *
                                       (parse->terrainFeatures->
                                        m_intersections[count].
                                        num_road_segments - i - 1));
                                MEM_free(tempRoadSegIDs);
                                i++;
                            }
                            break;
                        }
                        tempRoadSeg = ((TerrainFeaturesRoadLinks *)
                                       tempRoadSeg->roadSegmentVar)->nextRoad;
                    }
                    if (tempRoadSeg == NULL) {
                        sprintf(errorBuf, "Intersection %d not found in "
                                "corresponding road "
                                "segment chain for road segment %d\n",
                                parse->terrainFeatures->
                                m_intersections[count].ID,
                                parse->terrainFeatures->
                                m_intersections[count].roadSegments[i]);
                        ERROR_ReportError(errorBuf);
                    }
                }
            }
        }
    }
    //Fill out the "neighbors" variable for all the intersections
    for (count = 0;
            count < parse->terrainFeatures->m_numIntersections;
            count++)
    {
        parse->terrainFeatures->m_intersections[count].neighbors =
            (IntersectionID *)
            MEM_malloc(sizeof(IntersectionID) * parse->terrainFeatures->
                       m_intersections[count].num_road_segments);

        for (i = 0;
                i < parse->terrainFeatures->m_intersections[count].num_road_segments;
                i++)
        {
            if (parse->terrainFeatures->m_roadSegments
                    [parse->terrainFeatures->
                     m_intersections[count].roadSegments[i]].
                    firstIntersection == parse->terrainFeatures->
                    m_intersections[count].ID)
            {
                parse->terrainFeatures->m_intersections[count].neighbors[i] =
                    parse->terrainFeatures->m_roadSegments
                    [parse->terrainFeatures->
                     m_intersections[count].roadSegments[i]].
                    secondIntersection;
            }
            else if (parse->terrainFeatures->m_roadSegments
                     [parse->terrainFeatures->
                      m_intersections[count].roadSegments[i]].
                     secondIntersection == parse->terrainFeatures->
                     m_intersections[count].ID)
            {
                parse->terrainFeatures->m_intersections[count].neighbors[i] =
                    parse->terrainFeatures->m_roadSegments
                    [parse->terrainFeatures->
                     m_intersections[count].roadSegments[i]].
                    firstIntersection;
            }
            else {
                sprintf(errorBuf,
                        "Error while finding neighbors of intersection %s",
                        parse->terrainFeatures->
                        m_intersections[count].XML_ID);

                ERROR_ReportError(errorBuf);
            }
        }
    }

    double trafficLightProbability;

    IO_ReadDouble(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "TERRAIN-FEATURES-TRAFFIC-LIGHT-PROBABILITY",
        &wasFound,
        &trafficLightProbability);

    //if set, override file settings with probablistic traffic lights
    if (wasFound)
    {
        int seedVal;

        IO_ReadInt(
            ANY_NODEID,
            ANY_ADDRESS,
            nodeInput,
            "TERRAIN-FEATURES-TRAFFIC-LIGHT-SEED",
            &wasFound,
            &seedVal);

        if (wasFound == FALSE) {
            ERROR_ReportError("\"TERRAIN-FEATURES-TRAFFIC-SEED\" "
                              "needs to be specified.");
        }

        // use seed independent of global seed to keep traffic lights stable
        RandomDistribution<double> random;
        random.setSeed(seedVal);
        random.setDistributionUniform(0.0, 1.0);

        for (i = 0; i < terrainFeatures->m_numIntersections; i++)
        {
            Intersection* currentIntersection = &(terrainFeatures->m_intersections[i]);

            if (currentIntersection->intersectionVar != NULL)
            {
                MEM_free(currentIntersection->intersectionVar);
            }

            if (random.getRandomNumber() < trafficLightProbability)
            {
                currentIntersection->hasTrafficSignal = TRUE;
                currentIntersection->signalGroups =
                    (SignalGroupNumber*)
                    MEM_malloc(
                        sizeof(SignalGroupNumber)
                        * currentIntersection->num_road_segments);
            }
            else
            {
                currentIntersection->hasTrafficSignal = FALSE;
                currentIntersection->signalGroups = NULL;
            }
        }
        terrainFeatures->groupSignals();
    }
    else
    {
        int k;

        for (i = 0; i < terrainFeatures->m_numIntersections; i++)
        {
            Intersection* currentIntersection = &(terrainFeatures->m_intersections[i]);
            if (currentIntersection->intersectionVar != NULL)
            {
                currentIntersection->signalGroups =
                    (SignalGroupNumber*)
                    MEM_malloc(
                        sizeof(SignalGroupNumber)
                        * currentIntersection->num_road_segments);

                for (j = 0; j < currentIntersection->num_road_segments; j++)
                {
                    BOOL found = FALSE;
                    for (k = 0; k < currentIntersection->num_road_segments; k++)
                    {
                        if (((TrafficLight*) (currentIntersection->intersectionVar))[j].roadID == currentIntersection->roadSegments[k])
                        {
                            currentIntersection->signalGroups[k] = ((TrafficLight*) (currentIntersection->intersectionVar))[j].signalGroup;
                            found = TRUE;
                            break;
                        }
                    }
                    if (found == FALSE)
                    {
                        sprintf(errorBuf, "Intersection %s: reference %s under <synchronizedSignals> is not matched by a reference under <objects>\n", currentIntersection->XML_ID, terrainFeatures->m_roadSegments[((TrafficLight*) (currentIntersection->intersectionVar))[j].roadID].XML_ID);
                        ERROR_ReportError(errorBuf);
                    }
                }
                MEM_free(currentIntersection->intersectionVar);
            }
            else
            {
                currentIntersection->signalGroups = NULL;
            }
        }
    }

    //Clean up roadsegmentvar variables
    for (count = 0;
            count < parse->terrainFeatures->m_numRoadSegments;
            count++)
    {
        if (parse->terrainFeatures->
                m_roadSegments[count].roadSegmentVar != NULL) {
            MEM_free(parse->terrainFeatures->
                     m_roadSegments[count].roadSegmentVar);
            parse->terrainFeatures->
            m_roadSegments[count].roadSegmentVar = NULL;
        }
    }
    if (parse->terrainFeatures->m_numIntersections !=
            parse->maxIntersections)
    {
        ERROR_ReportError("Incorrect number of intersections parsed during "
                          "second pass");
    }
    //Print out intersection data
    if (DEBUG) {
        for (count = 0;
                count < parse->terrainFeatures->m_numIntersections;
                count++)
        {
            printf("intersection %d:\n\n", count);
            printf("\t%s\n", parse->terrainFeatures->
                   m_intersections[count].XML_ID);
            printf("\tlocation %lf %lf %lf, %d, %d, %d\n",
                   parse->terrainFeatures->
                   m_intersections[count].position.common.c1,
                   parse->terrainFeatures->
                   m_intersections[count].position.common.c2,
                   parse->terrainFeatures->
                   m_intersections[count].position.common.c3,
                   parse->terrainFeatures->
                   m_intersections[count].entranceType,
                   parse->terrainFeatures->
                   m_intersections[count].park,
                   parse->terrainFeatures->
                   m_intersections[count].station);
            if (parse->terrainFeatures->m_intersections[count].hasTrafficSignal)
            {
                printf("\tSynchronized Signal Groups: %d\n", parse->terrainFeatures->m_intersections[count].numSignalGroups);
                for (i = 0; i < parse->terrainFeatures->
                        m_intersections[count].num_road_segments; i++)
                {
                    printf("\tSegment %d: %s, %d, Signal Group %d\n", i,
                           parse->terrainFeatures->m_roadSegments
                           [parse->terrainFeatures->m_intersections[count].
                            roadSegments[i]].XML_ID,
                           parse->terrainFeatures->m_roadSegments
                           [parse->terrainFeatures->m_intersections[count].
                            roadSegments[i]].ID,
                           parse->terrainFeatures->m_intersections[count].
                           signalGroups[i]);
                }
            }
            else
            {
                printf("\tSynchronized Signal Groups: None\n");
                for (i = 0; i < parse->terrainFeatures->
                        m_intersections[count].num_road_segments; i++)
                {
                    printf("\tSegment %d: %s, %d\n", i,
                           parse->terrainFeatures->m_roadSegments
                           [parse->terrainFeatures->m_intersections[count].
                            roadSegments[i]].XML_ID,
                           parse->terrainFeatures->m_roadSegments
                           [parse->terrainFeatures->m_intersections[count].
                            roadSegments[i]].ID);
                }
            }
            for (i = 0;
                    i < parse->terrainFeatures->
                    m_intersections[count].num_road_segments;
                    i++)
            {
                printf("\tNeighbor %d: %s\n", i,
                       parse->terrainFeatures->m_intersections
                       [parse->terrainFeatures->m_intersections[count].
                        neighbors[i]].XML_ID);
            }
        }
    }
    if (parse->terrainFeatures->m_numParks != parse->maxParks)
    {
        ERROR_ReportError("Incorrect number of parks parsed during "
                          "second pass");
    }
    //Print out park data
    if (DEBUG) {
        for (count = 0;
                count < parse->terrainFeatures->m_numParks;
                count++)
        {
            printf("Park %d:\n\n", count+1);
            printf("\t%s\n",
                   parse->terrainFeatures->m_parks[count].XML_ID);
            printf("\tRep: %lf %lf %lf\n",
                   parse->terrainFeatures->m_parks[count].
                   representative.common.c1,
                   parse->terrainFeatures->m_parks[count].
                   representative.common.c2,
                   parse->terrainFeatures->m_parks[count].
                   representative.common.c3);
            for (i = 0;
                    i < parse->terrainFeatures->m_parks[count].num_vertices;
                    i++)
            {
                printf("\tVertex %d: %lf %lf %lf, %d\n", i,
                       parse->terrainFeatures->m_parks[count].
                       vertices[i].common.c1,
                       parse->terrainFeatures->m_parks[count].
                       vertices[i].common.c2,
                       parse->terrainFeatures->m_parks[count].
                       vertices[i].common.c3,
                       parse->terrainFeatures->m_parks[count].
                       exitIntersections[i]);
            }
        }
    }
    if (parse->terrainFeatures->m_numStations != parse->maxStations)
    {
        ERROR_ReportError("Incorrect number of stations parsed during "
                          "second pass");
    }
    //Print out station data
    if (DEBUG) {
        for (count = 0;
                count < parse->terrainFeatures->m_numStations;
                count++)
        {
            printf("Station %d:\n\n", count+1);
            printf("\t%s\n",
                   parse->terrainFeatures->m_stations[count].XML_ID);
            printf("\tRep: %lf %lf %lf\n",
                   parse->terrainFeatures->m_stations[count].
                   representative.common.c1,
                   parse->terrainFeatures->m_stations[count].
                   representative.common.c2,
                   parse->terrainFeatures->m_stations[count].
                   representative.common.c3);
            for (i = 0;
                    i < parse->terrainFeatures->m_stations[count].num_vertices;
                    i++)
            {
                printf("\tVertex %d: %lf %lf %lf, %d\n", i,
                       parse->terrainFeatures->m_stations[count].
                       vertices[i].common.c1,
                       parse->terrainFeatures->m_stations[count].
                       vertices[i].common.c2,
                       parse->terrainFeatures->m_stations[count].
                       vertices[i].common.c3,
                       parse->terrainFeatures->m_stations[count].
                       exitIntersections[i]);
            }
        }
    }
    if (DEBUG)
    {
        //Print out building data
        for (count = 0;
                count < parse->terrainFeatures->m_numBuildings;
                count++)
        {
            printf("Building %d:\n\n", count+1);
            printf("\t%s %s\n",
                   parse->terrainFeatures->m_buildings[count].XML_ID,
                   parse->terrainFeatures->m_buildings[count].featureName);
            for (i = 0;
                    i < parse->terrainFeatures->m_buildings[count].num_faces;
                    i++)
            {
                printf("\tFace %d:\n\n", i+1);
                printf("\t\t%s %lf\n",
                       parse->terrainFeatures->m_buildings[count].
                       faces[i].XML_ID,
                       parse->terrainFeatures->m_buildings[count].
                       faces[i].thickness);
                for (j = 0;
                        j < parse->terrainFeatures->m_buildings[count].
                        faces[i].num_vertices;
                        j++)
                {
                    printf("\t\tVert: %lf %lf %lf\n",
                           parse->terrainFeatures->m_buildings[count].
                           faces[i].vertices[j].common.c1,
                           parse->terrainFeatures->m_buildings[count].
                           faces[i].vertices[j].common.c2,
                           parse->terrainFeatures->m_buildings[count].
                           faces[i].vertices[j].common.c3);
                }
            }
        }
    }
    if (DEBUG)
    {
        //Print out foliage data
        for (count = 0;
                count < parse->terrainFeatures->m_numFoliage;
                count++)
        {
            printf("Foliage %d:\n\n", count+1);
            printf("\t%s %s\n",
                   parse->terrainFeatures->m_foliage[count].XML_ID,
                   parse->terrainFeatures->m_foliage[count].featureName);
            for (i = 0;
                    i < parse->terrainFeatures->m_foliage[count].num_faces;
                    i++)
            {
                printf("\tFace %d:\n\n", i+1);
                printf("\t\t%s %lf\n",
                       parse->terrainFeatures->m_foliage[count].
                       faces[i].XML_ID,
                       parse->terrainFeatures->m_foliage[count].
                       faces[i].thickness);
                for (j = 0;
                        j < parse->terrainFeatures->m_foliage[count].
                        faces[i].num_vertices;
                        j++)
                {
                    printf("\t\tVert: %lf %lf %lf\n",
                           parse->terrainFeatures->m_foliage[count].
                           faces[i].vertices[j].common.c1,
                           parse->terrainFeatures->m_foliage[count].
                           faces[i].vertices[j].common.c2,
                           parse->terrainFeatures->m_foliage[count].
                           faces[i].vertices[j].common.c3);
                }
            }
        }
    }
    //Check for hanging references
    for (j = 0; j < parse->hashTable->tableSize; j++) {
        if ((parse->hashTable->cells[j].entryState
                != EMPTY) &&
                (parse->hashTable->cells[j].objectState
                 != COMPLETE))
        {
            sprintf(errorBuf, "Incomplete Object: %s %d!!!",
                    parse->hashTable->
                    cells[j].XML_ID,
                    parse->hashTable->
                    cells[j].objectType);
            ERROR_ReportError(errorBuf);
        }
    }

    //Calculate lengths of road segments
    for (i = 0; i < terrainFeatures->m_numRoadSegments; i++)
    {
        double distance;
        RoadSegment* roadSegment = &(terrainFeatures->m_roadSegments[i]);
        roadSegment->length = 0;

        Coordinates* current = roadSegment->vertices;
        Coordinates* next;

        for (j = 1; j < roadSegment->num_vertices; j++)
        {
            next = &(roadSegment->vertices[j]);
            // should depend on system type
            COORD_CalcDistance(terrainData->getCoordinateSystem(),
                               current,
                               next,
                               &distance);
            roadSegment->length += distance;
            current = next;
        }
    }

    if (terrainFeatures->m_numBuildings > 0)
    {
        if (terrainFeatures->m_setFeaturesToGround == TRUE
                && terrainData->getCoordinateSystem() == LATLONALT)
        {
            terrainFeatures->setTerrainFeaturesToGround(
                terrainFeatures->m_buildings,
                terrainFeatures->m_numBuildings);
        }

        terrainFeatures->m_minBuildingHeight =
            terrainFeatures->m_maxBuildingHeight =
                (float)terrainFeatures->calculateBuildingHeight(terrainFeatures->m_buildings);
        terrainFeatures->m_totalBuildingHeight = 0.0;

        for (i = 0; i < terrainFeatures->m_numBuildings; i++)
        {
            terrainFeatures->addHeightInformation(
                &(terrainFeatures->m_buildings[i]),
                &terrainFeatures->m_minBuildingHeight,
                &terrainFeatures->m_maxBuildingHeight,
                &terrainFeatures->m_totalBuildingHeight);
            if (terrainData->getCoordinateSystem() == LATLONALT)
            {
                terrainFeatures->convertBuildingCoordinates(&(terrainFeatures->m_buildings[i]), GEOCENTRIC_CARTESIAN);
            }
            terrainFeatures->createPlaneParameters(&(terrainFeatures->m_buildings[i]));
            terrainFeatures->createBoundingCube(&(terrainFeatures->m_buildings[i]));
        }
    }//if//

    if (terrainFeatures->m_numFoliage > 0)
    {
        if (terrainFeatures->m_setFeaturesToGround == TRUE
                && terrainData->getCoordinateSystem() == LATLONALT)
        {
            terrainFeatures->setTerrainFeaturesToGround(
                terrainFeatures->m_foliage,
                terrainFeatures->m_numFoliage);
        }

        terrainFeatures->m_minFoliageHeight =
            terrainFeatures->m_maxFoliageHeight =
                (float)terrainFeatures->calculateBuildingHeight(terrainFeatures->m_foliage);
        terrainFeatures->m_totalFoliageHeight = 0.0;

        for (i = 0; i < terrainFeatures->m_numFoliage; i++)
        {
            terrainFeatures->addHeightInformation(
                &(terrainFeatures->m_foliage[i]),
                &terrainFeatures->m_minFoliageHeight,
                &terrainFeatures->m_maxFoliageHeight,
                &terrainFeatures->m_totalFoliageHeight);
            if (terrainData->getCoordinateSystem() == LATLONALT)
            {
                terrainFeatures->convertBuildingCoordinates(&(terrainFeatures->m_foliage[i]), GEOCENTRIC_CARTESIAN);
            }
            terrainFeatures->createPlaneParameters(&(terrainFeatures->m_foliage[i]));
            terrainFeatures->createBoundingCube(&(terrainFeatures->m_foliage[i]));
        }
    }

    //Free all auxiliary structures
    XML_ParserFree(parser);
    DestroyTable(parse);
    MEM_free(parse->coordinateList);
    MEM_free(parse->roadIDs);
    MEM_free(parse->trafficLights);
    MEM_free(parse->intIDs);
    MEM_free(parse->faces);
    MEM_free(parse->foliageFaces);
    MEM_free(parse);
}
